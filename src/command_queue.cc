#include "command_queue.h"

namespace dramsim3 {

CommandQueue::CommandQueue(int channel_id, const Config& config,
                           const ChannelState& channel_state, Statistics& stats)
    : clk_(0),
      rank_queues_empty(std::vector<bool>(config.ranks, true)),
      rank_idle_since(std::vector<uint64_t>(config.ranks, 0)),
      config_(config),
      channel_state_(channel_state),
      stats_(stats),
      next_rank_(0),
      next_bg_(0),
      next_bank_(0),
      next_queue_index_(0),
      queue_size_(static_cast<size_t>(config_.cmd_queue_size)),
      channel_id_(channel_id) {
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
    unsigned i = queues_.size();
    while (i > 0){
        IterateNext();
        if (channel_state_.IsRefreshWaiting(next_rank_, next_bg_, next_bank_)) {
            bool row_open = channel_state_.IsRowOpen(next_rank_, next_bg_,
                                                     next_bank_);
            int hit_count = channel_state_.RowHitCount(next_rank_, next_bg_,
                                                        next_bank_);
            if (row_open && hit_count == 0) {  // just open, finish this one
                // queues_[next_queue_index_];
                return GetFristReadyInBank(next_rank_, next_bg_, next_bank_);
            } else {
                auto addr = Address(0, next_rank_, next_bg_, next_bank_, -1, 0);
                Command ref = Command(CommandType::REFRESH, addr, -1);
                return channel_state_.GetRequiredCommand(ref);
            }
        } else {
            auto cmd = GetFristReadyInQueue(queues_[next_queue_index_]);
            if (cmd.IsValid()) {
                return cmd;
            }
        }
        i--;
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
        64;
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
        rank_queues_empty[cmd.Rank()] = false;
        return true;
    } else {
        return false;
    }
}

inline void CommandQueue::IterateNext() {
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
    next_queue_index_ = GetQueueIndex(next_rank_, next_bg_, next_bank_);
    return;
}

int CommandQueue::GetQueueIndex(int rank, int bankgroup, int bank) {
    if (queue_structure_ == QueueStructure::PER_RANK) {
        return rank;
    } else {
        return rank * config_.banks + bankgroup * config_.banks_per_group +
               bank;
    }
}

std::vector<Command>& CommandQueue::GetQueue(int rank, int bankgroup,
                                             int bank) {
    int index = GetQueueIndex(rank, bankgroup, bank);
    return queues_[index];
}

Command CommandQueue::GetFristReadyInQueue(std::vector<Command>& queue) {
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        // DO NOT REORDER R/W
        if (cmd_it->IsRead() != queue.begin()->IsRead()) {
            // TODO had to comment this out to not block 
            // but then what's the point of using separate queues?
            // break;
        }
        Command cmd = channel_state_.GetRequiredCommand(*cmd_it);
        // TODO required might be different from cmd_it, e.g. ACT vs READ
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


Command CommandQueue::GetFristReadyInBank(int rank, int bankgroup, int bank) {
    // only useful in rank queue
    auto& queue = GetQueue(rank, bankgroup, bank);
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        if (cmd_it->Rank() == rank && cmd_it->Bankgroup() == bankgroup &&
            cmd_it->Bank() == bank) {
            Command cmd = channel_state_.GetRequiredCommand(*cmd_it);
            if (channel_state_.IsReady(cmd, clk_)) {
                return cmd;
            }
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

}  // namespace dramsim3