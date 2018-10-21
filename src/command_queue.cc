#include "command_queue.h"

namespace dramsim3 {

CommandQueue::CommandQueue(int channel_id, const Config& config,
                           const ChannelState& channel_state, Statistics& stats)
    : rank_q_empty(config.ranks, true),
      config_(config),
      channel_state_(channel_state),
      stats_(stats),
      next_rank_(0),
      next_bg_(0),
      next_bank_(0),
      queue_size_(static_cast<size_t>(config_.cmd_queue_size)),
      clk_(0) {
    int num_queues = 0;
    if (config_.queue_structure == "PER_BANK") {
        queue_structure_ = QueueStructure::PER_BANK;
        num_queues = config_.banks * config_.ranks;
    } else if (config_.queue_structure == "PER_RANK") {
        queue_structure_ = QueueStructure::PER_RANK;
        num_queues = config_.ranks;
    } else {
        std::cerr << "Unsupportted queueing structure "
                  << config_.queue_structure << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    queues_.reserve(num_queues);
    for (int i = 0; i < num_queues; i++) {
        auto cmd_queue = std::vector<Command>();
        cmd_queue.reserve(config_.cmd_queue_size);
        queues_.push_back(cmd_queue);
    }
}

Command CommandQueue::GetCommandToIssue() {
    // prioritize refresh
    // Rank queues, bank queues, ref and refbs, so complicated
    if (channel_state_.IsRefreshWaiting()) {
        auto ref = channel_state_.PendingRefCommand();
        if (ref.cmd_type == CommandType::REFRESH) {
            for (size_t i = 0; i < queues_.size(); i++) {
                auto& queue = GetNextQueue();
                if (next_rank_ == ref.Rank()) {
                    for (auto it=queue.begin(); it != queue.end(); it++) {
                        auto cmd = PrepRefCmd(it, ref);
                        if (cmd.IsValid()) {
                            return cmd;
                        }
                    }
                }
            }
        } else {  // Bank level refresh
            auto& queue = GetQueue(ref.Rank(), ref.Bankgroup(), ref.Bank());
            for (auto it = queue.begin(); it != queue.end(); it++) {
                if (it->Rank() == ref.Rank() &&
                    it->Bankgroup() == ref.Bankgroup() &&
                    it->Bank() == ref.Bank()) {
                    auto cmd = PrepRefCmd(it, ref);
                    if (cmd.IsValid()) {
                        return cmd;
                    }
                }
            }
        }

        // Cannot find any ref-related cmd, (no returning till here), then
        // try to find other things to do in other ranks
        for (size_t i = 0; i < queues_.size(); i++) {
            if (next_rank_ != ref.Rank()) {  // not the refreshed rank
                auto& next_queue = GetQueue(next_rank_, next_bg_, next_bank_);
                auto cmd = GetFristReadyInQueue(next_queue);
                if (cmd.IsValid()) {
                    return cmd;
                }
            }
            GetNextQueue();
        }
    } else {
        int i = queues_.size();
        while (i > 0) {
            auto& queue = GetNextQueue();
            auto cmd = GetFristReadyInQueue(queue);
            if (cmd.IsValid()) {
                return cmd;
            }
            i--;
        }
    }
    return Command();
}

bool CommandQueue::ArbitratePrecharge(const Command& cmd) {
    auto& queue = GetQueue(cmd.Rank(), cmd.Bankgroup(), cmd.Bank());

    CMDIterator cmd_it;
    for (auto it = queue.begin(); it != queue.end(); it++) {
        if (it->id == cmd.id) {
            cmd_it = it;
            break;
        }
    }

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

bool CommandQueue::WillAcceptCommand(int rank, int bankgroup, int bank) {
    const auto& queue = GetQueue(rank, bankgroup, bank);
    return queue.size() < queue_size_;
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
    if (queue_structure_ == QueueStructure::PER_BANK) {
        next_bg_ = (next_bg_ + 1) % config_.bankgroups;
        if (next_bg_ == 0) {
            next_bank_ = (next_bank_ + 1) % config_.banks_per_group;
            if (next_bank_ == 0) {
                next_rank_ = (next_rank_ + 1) % config_.ranks;
            }
        }
    } else if (queue_structure_ == QueueStructure::PER_RANK) {
        next_rank_ = (next_rank_ + 1) % config_.ranks;
    } else {
        std::cerr << "Unknown queue structure" << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }
    return GetQueue(next_rank_, next_bg_, next_bank_);
}

int CommandQueue::GetQueueIndex(int rank, int bankgroup, int bank) {
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

Command CommandQueue::GetFristReadyInQueue(CMDQueue& queue) {
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        // TODO not enforcing R/W orders?
        Command cmd = channel_state_.GetRequiredCommand(*cmd_it);
        if (channel_state_.IsReady(cmd, clk_)) {
            if (cmd.cmd_type == CommandType::PRECHARGE) {
                if (!ArbitratePrecharge(cmd)) {
                    continue;
                }
            }
            return cmd;
        }
    }
    return Command();
}

void CommandQueue::IssueRWCommand(const Command& cmd) {
    auto& queue = GetQueue(cmd.Rank(), cmd.Bankgroup(), cmd.Bank());
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        if (cmd.id == cmd_it->id) {
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

Command CommandQueue::PrepRefCmd(CMDIterator& it, Command& ref) {
    int r = it->Rank();
    int g = it->Bankgroup();
    int b = it->Bank();
    bool row_open = channel_state_.IsRowOpen(r, g, b);
    int hit_count = channel_state_.RowHitCount(r, g, b);
    if (row_open && hit_count) {
        Command cmd = channel_state_.GetRequiredCommand(*it);
        if (channel_state_.IsReady(cmd, clk_)) {
            return cmd;
        }
    } else {  // precharge
        Command cmd = channel_state_.GetRequiredCommand(ref);
        if (channel_state_.IsReady(cmd, clk_)) {
            return cmd;
        }
    }
    return Command();
}

}  // namespace dramsim3