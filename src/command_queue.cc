#include "command_queue.h"

namespace dramsim3 {

CommandQueue::CommandQueue(int channel_id, const Config& config,
                           const ChannelState& channel_state, Statistics& stats)
    : rank_q_empty(config.ranks, true),
      config_(config),
      channel_state_(channel_state),
      stats_(stats),
      queue_size_(static_cast<size_t>(config_.cmd_queue_size)),
      queue_idx_(0),
      clk_(0) {
    if (config_.queue_structure == "PER_BANK") {
        queue_structure_ = QueueStructure::PER_BANK;
        num_queues_ = config_.banks * config_.ranks;
    } else if (config_.queue_structure == "PER_RANK") {
        queue_structure_ = QueueStructure::PER_RANK;
        num_queues_ = config_.ranks;
    } else {
        std::cerr << "Unsupportted queueing structure "
                  << config_.queue_structure << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    queues_.reserve(num_queues_);
    for (int i = 0; i < num_queues_; i++) {
        auto cmd_queue = std::vector<Command>();
        cmd_queue.reserve(config_.cmd_queue_size);
        queues_.push_back(cmd_queue);
    }
}

Command CommandQueue::GetCommandToIssue() {
    // prioritize refresh
    // Rank queues, bank queues, ref and refbs, so complicated
    Command cmd;
    if (channel_state_.IsRefreshWaiting()) {
        auto ref = channel_state_.PendingRefCommand();
        std::unordered_set<int> ref_q_idx = GetRefQIndices(ref);
        for (auto idx : ref_q_idx) {
            auto& queue = queues_[idx];
            for (auto it = queue.begin(); it != queue.end(); it++) {
                cmd = PrepRefCmd(it, ref);
                if (cmd.IsValid()) {
                    if (cmd.IsReadWrite()) {
                        EraseRWCommand(cmd);
                    }
                    return cmd;
                }
            }
        }
        // Cannot find any ref-related cmd, (no returning till here), then
        // try to find other things to do in other ranks
        for (int i = 0; i < num_queues_; i++) {
            if (ref_q_idx.count(i) == 0) {  // not a ref related queue
                auto& queue = queues_[i];
                cmd = GetFristReadyInQueue(queue);
                if (cmd.IsValid()) {
                    if (cmd.IsReadWrite()) {
                        EraseRWCommand(cmd);
                    }
                    return cmd;
                }
            }
        }
    } else {
        for (int i = 0; i < num_queues_; i++) {
            auto& queue = GetNextQueue();
            cmd = GetFristReadyInQueue(queue);
            if (cmd.IsValid()) {
                if (cmd.IsReadWrite()) {
                    EraseRWCommand(cmd);
                }
                return cmd;
            }
        }
    }
    return Command();
}

bool CommandQueue::ArbitratePrecharge(const CMDIterator& cmd_it,
                                      const CMDQueue& queue) const {
    auto cmd = *cmd_it;

    for (auto prev_itr = queue.begin(); prev_itr != cmd_it; prev_itr++) {
        if (prev_itr->Rank() == cmd.Rank() &&
            prev_itr->Bankgroup() == cmd.Bankgroup() &&
            prev_itr->Bank() == cmd.Bank()) {
            return false;
        }
    }

    bool pending_row_hits_exist = false;
    int open_row =
        channel_state_.OpenRow(cmd.Rank(), cmd.Bankgroup(), cmd.Bank());
    for (auto pending_itr = cmd_it; pending_itr != queue.end(); pending_itr++) {
        if (pending_itr->Row() == open_row &&
            pending_itr->Bank() == cmd.Bank() &&
            pending_itr->Bankgroup() == cmd.Bankgroup() &&
            pending_itr->Rank() == cmd.Rank()) {
            pending_row_hits_exist = true;
            break;
        }
    }

    bool rowhit_limit_reached =
        channel_state_.RowHitCount(cmd.Rank(), cmd.Bankgroup(), cmd.Bank()) >=
        4;
    if (!pending_row_hits_exist || rowhit_limit_reached) {
        stats_.num_ondemand_pres++;
        return true;
    }
    return false;
}

bool CommandQueue::WillAcceptCommand(int rank, int bankgroup, int bank) const {
    int q_idx = GetQueueIndex(rank, bankgroup, bank);
    return queues_[q_idx].size() < queue_size_;
}

bool CommandQueue::AddCommand(Command cmd) {
    auto& queue = GetQueue(cmd.Rank(), cmd.Bankgroup(), cmd.Bank());
    if (queue.size() < queue_size_) {
        queue.push_back(cmd);
        rank_q_empty[cmd.Rank()] = false;
        return true;
    } else {
        return false;
    }
}

CMDQueue& CommandQueue::GetNextQueue() {
    queue_idx_++;
    if (queue_idx_ == num_queues_) {
        queue_idx_ = 0;
    }
    return queues_[queue_idx_];
}

std::unordered_set<int> CommandQueue::GetRefQIndices(const Command& ref) const {
    std::unordered_set<int> ref_q_indices;
    if (ref.cmd_type == CommandType::REFRESH) {
        if (queue_structure_ == QueueStructure::PER_BANK) {
            for (int i = 0; i < num_queues_; i++) {
                if (i / config_.banks != ref.Rank()) {
                    ref_q_indices.insert(i);
                }
            }
        } else {
            ref_q_indices.insert(ref.Rank());
        }
    } else {  // refb
        int idx = GetQueueIndex(ref.Rank(), ref.Bankgroup(), ref.Bank());
        ref_q_indices.insert(idx);
    }
    return ref_q_indices;
}

int CommandQueue::GetQueueIndex(int rank, int bankgroup, int bank) const {
    if (queue_structure_ == QueueStructure::PER_RANK) {
        return rank;
    } else {
        return rank * config_.banks + bankgroup * config_.banks_per_group +
               bank;
    }
}

CMDQueue& CommandQueue::GetQueue(int rank, int bankgroup, int bank) {
    int index = GetQueueIndex(rank, bankgroup, bank);
    return queues_[index];
}

Command CommandQueue::GetFristReadyInQueue(CMDQueue& queue) const {
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        Command cmd = channel_state_.GetRequiredCommand(*cmd_it);
        if (channel_state_.IsReady(cmd, clk_)) {
            if (cmd.cmd_type == CommandType::PRECHARGE) {
                if (!ArbitratePrecharge(cmd_it, queue)) {
                    continue;
                }
            } else if (cmd.IsWrite()) {
                if (HasRWDependency(cmd_it, queue)) {
                    continue;
                }
            }
            return cmd;
        }
    }
    return Command();
}

void CommandQueue::EraseRWCommand(const Command& cmd) {
    auto& queue = GetQueue(cmd.Rank(), cmd.Bankgroup(), cmd.Bank());
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        if (cmd.hex_addr == cmd_it->hex_addr) {
            queue.erase(cmd_it);
            return;
        }
    }
    std::cerr << "cannot find cmd!" << std::endl;
    exit(1);
}

int CommandQueue::QueueUsage() const {
    int usage = 0;
    for (auto i = queues_.begin(); i != queues_.end(); i++) {
        usage += i->size();
    }
    return usage;
}

bool CommandQueue::HasRWDependency(const CMDIterator& cmd_it,
                                   const CMDQueue& queue) const {
    // Read after write has been checked in controller so we only
    // check write after read here
    for (auto it = queue.begin(); it != cmd_it; it++) {
        if (it->IsRead() && it->Row() == cmd_it->Row() &&
            it->Column() == cmd_it->Column() && it->Bank() == cmd_it->Bank() &&
            it->Bankgroup() == cmd_it->Bankgroup()) {
            stats_.num_wr_dependency++;
            return true;
        }
    }
    return false;
}

Command CommandQueue::PrepRefCmd(const CMDIterator& it,
                                 const Command& ref) const {
    int r = it->Rank();
    int g = it->Bankgroup();
    int b = it->Bank();
    // if bank-level ref, and it is from a rank queue, ignore
    if (ref.cmd_type == CommandType::REFRESH_BANK) {
        if (g != ref.Bankgroup() || b != ref.Bank()) {
            return Command();
        }
    }
    bool row_open = channel_state_.IsRowOpen(r, g, b);
    int hit_count = channel_state_.RowHitCount(r, g, b);
    if (row_open && hit_count) {
        Command cmd = channel_state_.GetRequiredCommand(*it);
        if (channel_state_.IsReady(cmd, clk_)) {
            return cmd;
        }
    } else {  // precharge or refresh
        Command cmd = channel_state_.GetRequiredCommand(ref);
        if (channel_state_.IsReady(cmd, clk_)) {
            return cmd;
        }
    }
    return Command();
}

}  // namespace dramsim3
