#include "command_queue.h"
#include "statistics.h"

namespace dramsim3 {

CommandQueue::CommandQueue(int channel_id, const Config& config,
                           const ChannelState& channel_state, Statistics& stats)
    : clk_(0),
      rank_queues_empty(std::vector<bool>(config.ranks, true)),
      rank_queues_empty_from_time_(std::vector<uint64_t>(config.ranks, 0)),
      config_(config),
      channel_state_(channel_state),
      stats_(stats),
      next_rank_(0),
      next_bankgroup_(0),
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
    for (unsigned i = 0; i < queues_.size(); i++) {
        // auto cmd = GetCommandToIssueFromQueue(queues_[next_queue_index_]);
        IterateNext();
        auto cmd = GetFristReadyInQueue(queues_[next_queue_index_]);
        if (!cmd.IsValid()) {
            continue;
        } else {
            return cmd;
        }
    }
    return Command();
}

Command CommandQueue::GetCommandToIssueFromQueue(std::vector<Command>& queue) {
    // Prioritize row hits while honoring read, write dependencies
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        Command cmd = channel_state_.GetRequiredCommand(*cmd_it);
        // TODO - Can specizalize for PER_BANK queues (simulator speed)
        if (channel_state_.IsReady(cmd, clk_)) {
            if (cmd.IsReadWrite()) {
                // Check for read/write dependency check. Necessary only for
                // unified queues
                bool dependency = false;
                for (auto dep_itr = queue.begin(); dep_itr != cmd_it;
                        dep_itr++) {
                    if (dep_itr->Row() == cmd.Row()) {
                        dependency = true;
                        break;
                    }
                }
                if (!dependency) {
                    return cmd;
                }
            } else if (cmd.cmd_type == CommandType::PRECHARGE) {
                // Attempt to issue a precharge only if
                // 0. There are no prior requests to the same bank in the
                // queue (and)
                // 1. There are no pending row hits to the open row in the
                // bank (or)
                // 2. There are pending row hits to the open row but the max
                // allowed cap for row hits has been exceeded

                bool prior_requests_to_bank_exist = false;
                for (auto prior_itr = queue.begin(); prior_itr != cmd_it;
                        prior_itr++) {
                    if (prior_itr->Bank() == cmd.Bank() &&
                        prior_itr->Bankgroup() == cmd.Bankgroup() &&
                        prior_itr->Rank() == cmd.Rank()) {
                        prior_requests_to_bank_exist = true;
                        break;
                    }
                }
                bool pending_row_hits_exist = false;
                auto open_row = channel_state_.OpenRow(
                    cmd.Rank(), cmd.Bankgroup(), cmd.Bank());
                for (auto pending_itr = cmd_it; pending_itr != queue.end();
                        pending_itr++) {
                    if (pending_itr->Row() == open_row &&
                        pending_itr->Bank() == cmd.Bank() &&
                        pending_itr->Bankgroup() == cmd.Bankgroup() &&
                        pending_itr->Rank() == cmd.Rank()) {
                        pending_row_hits_exist = true;
                        break;
                    }
                }
                bool rowhit_limit_reached =
                    channel_state_.RowHitCount(cmd.Rank(), cmd.Bankgroup(),
                                                cmd.Bank()) >= 4;
                if (!prior_requests_to_bank_exist &&
                    (!pending_row_hits_exist || rowhit_limit_reached)) {
                    stats_.numb_ondemand_precharges++;
                    return cmd;
                }
            } else {
                return cmd;
            }
        }
    }
    return Command();
}

Command CommandQueue::AggressivePrecharge() {
    // TODO - Why such round robin order?
    for (auto i = 0; i < config_.ranks; i++) {
        for (auto j = 0; j < config_.bankgroups; j++) {
            for (auto k = 0; k < config_.banks_per_group; k++) {
                if (channel_state_.IsRowOpen(i, j, k)) {
                    auto cmd = Command(CommandType::PRECHARGE,
                                       Address(-1, i, j, k, -1, -1), -1);
                    if (channel_state_.IsReady(cmd, clk_)) {
                        bool pending_row_hits_exist = false;
                        auto open_row = channel_state_.OpenRow(i, j, k);
                        auto& queue = GetQueue(i, j, k);
                        for (auto const &pending_req : queue) {
                            if (pending_req.Row() == open_row &&
                                pending_req.Bank() == k &&
                                pending_req.Bankgroup() == j &&
                                pending_req.Rank() == i) {
                                pending_row_hits_exist =
                                    true;  // Entire address upto row matches
                                break;
                            }
                        }
                        if (!pending_row_hits_exist) {
                            stats_.numb_aggressive_precharges++;
                            return Command(
                                CommandType::PRECHARGE,
                                Address(channel_id_, i, j, k, -1, -1), -1);
                        }
                    }
                }
            }
        }
    }
    return Command();
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
        next_bankgroup_ = (next_bankgroup_ + 1) % config_.bankgroups;
        if (next_bankgroup_ == 0) {
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
    next_queue_index_ = GetQueueIndex(next_rank_, next_bankgroup_, next_bank_);
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
        Command cmd = channel_state_.GetRequiredCommand(*cmd_it);
        // TODO required might be different from cmd_it, e.g. ACT vs READ
        if (channel_state_.IsReady(cmd, clk_)) {
            return cmd;
        }
    }
    return Command();
}

CMDIterator CommandQueue::GetFirstRWInQueue(CMDQueue& queue) {
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        int rank, bankgroup, bank;
        rank = cmd_it->Rank();
        bankgroup = cmd_it->Bankgroup();
        bank = cmd_it->Bank();
        if (channel_state_.OpenRow(rank, bankgroup, bank) != -1) {
            if (channel_state_.IsReady(*cmd_it, clk_)) {
                return cmd_it;
            }
        }
    }
    return queue.end();
}

Command CommandQueue::GetFristReadyInBank(int rank, int bankgroup, int bank) {
    // only useful in rank queue
    auto &queue = GetQueue(rank, bankgroup, bank);
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
}

int CommandQueue::QueueUsage() const {
    int usage = 0;
    for (auto i = queues_.begin(); i != queues_.end(); i++) {
        usage += i->size();
    }
    return usage;
}

}  // namespace dramsim3