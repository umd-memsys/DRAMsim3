#include "command_queue.h"
#include "statistics.h"

using namespace std;
using namespace dramcore;

CommandQueue::CommandQueue(uint32_t channel_id, const Config& config,
                           const ChannelState& channel_state, Statistics& stats)
    : clk_(0),
      rank_queues_empty(vector<bool>(config.ranks, true)),
      rank_queues_empty_from_time_(std::vector<uint64_t>(config.ranks, 0)),
      config_(config),
      channel_state_(channel_state),
      stats_(stats),
      next_rank_(0),
      next_bankgroup_(0),
      next_bank_(0),
      next_queue_index_(0),
      channel_id_(channel_id) {
    int num_queues = 0;
    if (config_.queue_structure == "PER_BANK") {
        queue_structure_ = QueueStructure::PER_BANK;
        num_queues = config_.banks * config_.ranks;
    } else if (config_.queue_structure == "PER_RANK") {
        queue_structure_ = QueueStructure::PER_RANK;
        num_queues = config_.ranks;
    } else {
        cerr << "Unsupportted queueing structure " << config_.queue_structure
             << endl;
        AbruptExit(__FILE__, __LINE__);
    }

    queues_.reserve(num_queues);
    for (int i = 0; i < num_queues; i++) {
        queues_.push_back(std::list<Request*>());
    }
}

Command CommandQueue::GetCommandToIssue() {
    for (unsigned i = 0; i < queues_.size(); i++) {
        auto cmd = GetCommandToIssueFromQueue(queues_[next_queue_index_]);
        IterateNext();
        if (cmd.IsValid()) {
            return cmd;
        }
    }
    return Command();
}

Command CommandQueue::GetCommandToIssueFromQueue(std::list<Request*>& queue) {
    // Prioritize row hits while honoring read, write dependencies
    for (auto req_itr = queue.begin(); req_itr != queue.end(); req_itr++) {
        auto req = *req_itr;
        if (!channel_state_.IsRefreshWaiting(req->Rank(), req->Bankgroup(),
                                             req->Bank())) {
            Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
            // TODO - Can specizalize for PER_BANK queues (simulator speed)
            if (channel_state_.IsReady(cmd, clk_)) {
                if (cmd.IsReadWrite()) {
                    // Check for read/write dependency check. Necessary only for
                    // unified queues
                    bool dependency = false;
                    for (auto dep_itr = queue.begin(); dep_itr != req_itr;
                         dep_itr++) {
                        auto dep = *dep_itr;
                        if (dep->Row() == cmd.Row()) {  // Just check the
                                                        // address to be more
                                                        // generic
                            // ASSERT the cmd are of different types. If one is
                            // read, other write. Vice versa.
                            dependency = true;
                            break;
                        }
                    }
                    if (!dependency) {
                        IssueRequest(queue, req_itr);
                        return cmd;
                    }
                } else if (cmd.cmd_type_ == CommandType::PRECHARGE) {
                    // Attempt to issue a precharge only if
                    // 1. There are no prior requests to the same bank in the
                    // queue (and)
                    // 1. There are no pending row hits to the open row in the
                    // bank (or)
                    // 2. There are pending row hits to the open row but the max
                    // allowed cap for row hits has been exceeded

                    bool prior_requests_to_bank_exist = false;
                    for (auto prior_itr = queue.begin(); prior_itr != req_itr;
                         prior_itr++) {
                        auto prior_req = *prior_itr;
                        if (prior_req->Bank() == cmd.Bank() &&
                            prior_req->Bankgroup() == cmd.Bankgroup() &&
                            prior_req->Rank() == cmd.Rank()) {
                            prior_requests_to_bank_exist =
                                true;  // Entire address upto bank matches
                            break;
                        }
                    }
                    bool pending_row_hits_exist = false;
                    auto open_row = channel_state_.OpenRow(
                        cmd.Rank(), cmd.Bankgroup(), cmd.Bank());
                    for (auto pending_itr = req_itr; pending_itr != queue.end();
                         pending_itr++) {  // TODO - req_itr + 1?
                        auto pending_req = *pending_itr;
                        if (pending_req->Row() == open_row &&
                            pending_req->Bank() == cmd.Bank() &&
                            pending_req->Bankgroup() == cmd.Bankgroup() &&
                            pending_req->Rank() == cmd.Rank()) {
                            pending_row_hits_exist =
                                true;  // Entire address upto row matches
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
                } else
                    return cmd;
            }
        }
    }
    return Command();
}

Command CommandQueue::AggressivePrecharge() {
    // TODO - Why such round robin order?
    for (auto i = 0; i < config_.ranks; i++) {
        for (auto k = 0; k < config_.banks_per_group; k++) {
            for (auto j = 0; j < config_.bankgroups; j++) {
                if (channel_state_.IsRowOpen(i, j, k)) {
                    auto cmd = Command(CommandType::PRECHARGE,
                                       Address(-1, i, j, k, -1, -1));
                    if (channel_state_.IsReady(cmd, clk_)) {
                        bool pending_row_hits_exist = false;
                        auto open_row = channel_state_.OpenRow(i, j, k);
                        auto& queue = GetQueue(i, j, k);
                        for (auto pending_req : queue) {
                            if (pending_req->Row() == open_row &&
                                pending_req->Bank() == k &&
                                pending_req->Bankgroup() == j &&
                                pending_req->Rank() == i) {
                                pending_row_hits_exist =
                                    true;  // Entire address upto row matches
                                break;
                            }
                        }
                        if (!pending_row_hits_exist) {
                            stats_.numb_aggressive_precharges++;
                            return Command(
                                CommandType::PRECHARGE,
                                Address(channel_id_, i, j, k, -1, -1));
                        }
                    }
                }
            }
        }
    }
    return Command();
}

bool CommandQueue::IsReqInsertable(Request* req) {
    std::list<Request*>& queue =
        GetQueue(req->Rank(), req->Bankgroup(), req->Bank());
    return queue.size() < config_.queue_size;
}

bool CommandQueue::InsertReq(Request* req) {
    std::list<Request*>& queue =
        GetQueue(req->Rank(), req->Bankgroup(), req->Bank());
    if (queue.size() < config_.queue_size) {
        queue.push_back(req);
        if (rank_queues_empty[req->Rank()]) {
            rank_queues_empty[req->Rank()] = false;
        }
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
        cerr << "Unknown queue structure\n";
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

std::list<Request*>& CommandQueue::GetQueue(int rank, int bankgroup, int bank) {
    int index = GetQueueIndex(rank, bankgroup, bank);
    return queues_[index];
}

void CommandQueue::IssueRequest(std::list<Request*>& queue,
                                std::list<Request*>::iterator req_itr) {
    // Put the requests into a new buffer. They will be returned to the CPU
    // after the read/write latency
    auto req = *req_itr;
    if (req->cmd_.IsRead()) {
        req->exit_time_ = clk_ + config_.read_delay;
        stats_.numb_read_reqs_issued++;
    } else if (req->cmd_.IsWrite()) {
        req->exit_time_ = clk_ + config_.write_delay;
        stats_.numb_write_reqs_issued++;
    } else {
        cerr << "Unknown request type\n" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
    issued_req_.splice(issued_req_.end(), queue, req_itr);

    // Update rank queues emptyness
    if (queue.empty() && !rank_queues_empty[req->Rank()]) {
        if (queue_structure_ == QueueStructure::PER_RANK) {
            rank_queues_empty[req->Rank()] = true;
            rank_queues_empty_from_time_[req->Rank()] = clk_;
        } else if (queue_structure_ == QueueStructure::PER_BANK) {
            auto empty = true;
            for (auto j = 0; j < config_.bankgroups; j++) {
                for (auto k = 0; k < config_.banks_per_group; k++) {
                    auto& bank_queue =
                        GetQueue(req->Rank(), req->Bankgroup(), req->Bank());
                    if (!bank_queue.empty()) {
                        empty = false;
                        break;
                    }
                }
                if (!empty) break;  // Achieving multi-loop break
            }
            if (empty) {
                rank_queues_empty[req->Rank()] = true;
                rank_queues_empty_from_time_[req->Rank()] = clk_;
            }
        } else {
            AbruptExit(__FILE__, __LINE__);
        }
    }
    return;
}

int CommandQueue::QueueUsage() const {
    int usage = 0;
    for (auto i = queues_.begin(); i != queues_.end(); i++) {
        usage += i->size();
    }
    return usage;
}
