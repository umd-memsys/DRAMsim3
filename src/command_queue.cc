#include "command_queue.h"

using namespace std;

CommandQueue::CommandQueue(const Config& config, const ChannelState& channel_state) :
    clk(0),
    config_(config),
    channel_state_(channel_state),
    next_rank_(0),
    next_bankgroup_(0),
    next_bank_(0),
    req_q_per_bank_(config_.ranks, std::vector< vector<std::list<Request*>> >(config_.bankgroups, std::vector<std::list<Request*>>(config_.banks_per_group, std::list<Request*>()) ) ),
    req_q_per_rank_(config_.ranks, list<Request*>())
{}

Command CommandQueue::GetCommandToIssue() {
    if(config_.queue_structure == "PER_BANK") {
        //Rank, Bank, Bankgroup traversal of queues
        for (auto i = 0; i < config_.ranks; i++) {
            for (auto k = 0; k < config_.banks_per_group; k++) {
                for (auto j = 0; j < config_.bankgroups; j++) {
                    auto &queue = GetQueue(next_rank_, next_bankgroup_, next_bank_);
                    IterateNext();
                    auto cmd = GetCommandToIssueFromQueue(queue);
                    if (cmd.IsValid())
                        return cmd;
                }
            }
        }
        return Command();
    }
    if(config_.queue_structure == "PER_RANK") {
        for (auto i = 0; i < config_.ranks; i++) {
            auto &queue = GetQueue(next_rank_, -1, -1);
            IterateNext();
            auto cmd = GetCommandToIssueFromQueue(queue);
            if(cmd.IsValid())
                return cmd;
        }
        return Command();
    }
    else {
        cerr << "Unknown queue structure\n";
        exit(-1);
    }

}

Command CommandQueue::GetCommandToIssueFromQueue(std::list<Request*>& queue) {
    //Prioritize row hits while honoring read, write dependencies
    for(auto req_itr = queue.begin(); req_itr != queue.end(); req_itr++) {
        auto req = *req_itr;
        Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
        //TODO - Can specizalize for PER_BANK queeus (simulator speed)
        if(channel_state_.IsReady(cmd, clk)) {
            if ( cmd.IsReadWrite()) {
                //Check for read/write dependency check. Necessary only for unified queues
                bool dependency = false;
                for(auto dep_itr = queue.begin(); dep_itr != req_itr; dep_itr++) {
                    auto dep = *dep_itr;
                    if( dep->Row() == cmd.Row()) { //Just check the address to be more generic
                        //ASSERT the cmd are of different types. If one is read, other write. Vice versa.
                        dependency = true;
                        break;
                    }
                }
                if(!dependency) {
                    //Sought of actually issuing the read/write command
                    delete(*req_itr);
                    queue.erase(req_itr);
                    return cmd;
                }
            }
            else if( cmd.cmd_type_ == CommandType::PRECHARGE) {
                // Attempt to issue a precharge only if
                // 1. There are no prior requests to the same bank in the queue (and)
                // 1. There are no pending row hits to the open row in the bank (or)
                // 2. There are pending row hits to the open row but the max allowed cap for row hits has been exceeded

                bool prior_requests_to_bank_exist = false;
                for(auto prior_itr = queue.begin(); prior_itr != req_itr; prior_itr++) {
                    auto prior_req = *prior_itr;
                    if( prior_req->Bank() == cmd.Bank() && prior_req->Bankgroup() == cmd.Bankgroup() && prior_req->Rank() == cmd.Rank()) {
                        prior_requests_to_bank_exist = true; //Entire address upto bank matches
                        break;
                    }
                }
                bool pending_row_hits_exist = false;
                auto open_row = channel_state_.OpenRow(cmd.Rank(), cmd.Bankgroup(), cmd.Bank());
                for(auto pending_itr = req_itr; pending_itr != queue.end(); pending_itr++) { //TODO - req_itr + 1?
                    auto pending_req = *pending_itr;
                    if( pending_req->Row() == open_row && pending_req->Bank() == cmd.Bank() &&
                        pending_req->Bankgroup() == cmd.Bankgroup() && pending_req->Rank() == cmd.Rank()) {
                        pending_row_hits_exist = true; //Entire address upto row matches
                        break;
                    }
                }
                bool rowhit_limit_reached = channel_state_.RowHitCount(cmd.Rank(), cmd.Bankgroup(), cmd.Bank()) >= 4;
                if( !prior_requests_to_bank_exist && (!pending_row_hits_exist || rowhit_limit_reached))
                    return cmd;
            }
            else
                return cmd;
        }
    }
    return Command();
}


Command CommandQueue::AggressivePrecharge() {
    //TODO - Why such round robin order?
    for(auto i = 0; i < config_.ranks; i++) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto j = 0; j < config_.bankgroups; j++) {
                if(channel_state_.IsRowOpen(i, j, k)) {
                    auto cmd = Command(CommandType::PRECHARGE, Address( -1, i, j, k, -1, -1));
                    if(channel_state_.IsReady(cmd, clk)) {
                        bool pending_row_hits_exist = false;
                        auto open_row = channel_state_.OpenRow(i, j, k);
                        auto& queue = GetQueue(i, j, k);
                        for(auto pending_req : queue) {
                            if( pending_req->Row() == open_row && pending_req->Bank() == k &&
                                pending_req->Bankgroup() == j && pending_req->Rank() == i) {
                                pending_row_hits_exist = true; //Entire address upto row matches
                                break;
                            }
                        }
                        if(!pending_row_hits_exist)
                            return Command(CommandType::PRECHARGE, Address(-1, i, j, k, -1, -1));
                    }
                }
            }
        }
    }
    return Command();
}

bool CommandQueue::InsertReq(Request* req) {
    if(config_.queue_structure == "PER_BANK") {
        auto r = req->Rank(), bg = req->Bankgroup(), b = req->Bank();
        if (req_q_per_bank_[r][bg][b].size() < config_.queue_size) {
            req_q_per_bank_[r][bg][b].push_back(req);
            return true;
        } else
            return false;
    }
    else if(config_.queue_structure == "PER_RANK") {
        auto r = req->Rank();
        if (req_q_per_rank_[r].size() < config_.queue_size) {
            req_q_per_rank_[r].push_back(req);
            return true;
        } else
            return false;
    }
    else {
        cerr << "Unknown queue structure\n";
        exit(-1);
    }
}

inline void CommandQueue::IterateNext() {
    if(config_.queue_structure == "PER_BANK") {
        next_bankgroup_ = (next_bankgroup_ + 1) % config_.bankgroups;
        if (next_bankgroup_ == 0) {
            next_bank_ = (next_bank_ + 1) % config_.banks_per_group;
            if (next_bank_ == 0) {
                next_rank_ = (next_rank_ + 1) % config_.ranks;
            }
        }
    }
    else if(config_.queue_structure == "PER_RANK") {
        next_rank_ = (next_rank_ + 1) % config_.ranks;
    }
    else {
        cerr << "Unknown queue structure\n";
        exit(-1);
    }
    return;
}

std::list<Request*>& CommandQueue::GetQueue(int rank, int bankgroup, int bank) {
    if(config_.queue_structure == "PER_BANK")
        return req_q_per_bank_[rank][bankgroup][bank];
    else if(config_.queue_structure == "PER_RANK")
        return req_q_per_rank_[rank];
    else {
        cerr << "Unknown queue structure\n";
        exit(-1);
    }
}