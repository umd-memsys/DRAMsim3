#include "command_queue.h"
#include <vector>
#include <list>

using namespace std;

CommandQueue::CommandQueue(const Config& config, const ChannelState& channel_state) :
    clk(0),
    config_(config),
    channel_state_(channel_state),
    next_rank_(0),
    next_bankgroup_(0),
    next_bank_(0),
    req_q_(config_.ranks, vector< vector<list<Request*>> >(config_.bankgroups, vector<list<Request*>>(config_.banks_per_group, list<Request*>()) ) )
{

}

Command CommandQueue::GetCommandToIssue() {
    //Rank, Bank, Bankgroup traversal of queues
    for(auto i = 0; i < config_.ranks; i++) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto j = 0; j < config_.bankgroups; j++) {
                if( !channel_state_.IsRefreshWaiting(next_rank_, next_bankgroup_, next_bank_) ) {
                    auto& queue = GetQueue(next_rank_, next_bankgroup_, next_bank_);
                    IterateNext();
                    //Prioritize row hits while honoring read, write dependencies
                    for(auto req_itr = queue.begin(); req_itr != queue.end(); req_itr++) {
                        auto req = *req_itr;
                        Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
                        //TODO - For per bank unified queues no need to process out of order (simulator speed)
                        if(channel_state_.IsReady(cmd, clk)) {
                            if ( req->cmd_.cmd_type_ == cmd.cmd_type_) { //TODO - Essentially checking for a row hit. Replace with IsReadWrite() function?
                                //Check for read/write dependency check. Necessary only for unified queues
                                bool dependency = false;
                                for(auto dep_itr = queue.begin(); dep_itr != req_itr; dep_itr++) {
                                    auto dep = *dep_itr;
                                    if( dep->cmd_.row_ == cmd.row_) { //Just check the address to be more generic
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
                                // Only attempt issuing a precharge if either one of the following conditions are satisfied
                                // 1. There are no pending row hits to the open row in the bank
                                // 2. There are pending row hits to the open row but the max allowed cap for row hits has been exceeded
                                bool pending_row_hits_exist = false;
                                auto open_row = channel_state_.OpenRow(cmd.rank_, cmd.bankgroup_, cmd.bank_);
                                for(auto pending_req : queue) {
                                    if( pending_req->cmd_.row_ == open_row) { //TODO - And same bank if queues are rank based
                                        pending_row_hits_exist = true;
                                        break;
                                    }
                                }
                                if( !pending_row_hits_exist || channel_state_.RowHitCount(cmd.rank_, cmd.bankgroup_, cmd.bank_) >= 4)
                                    return cmd;
                            }
                            else
                                return cmd;
                        }
                    }
                }
            }
        }
    }
    return Command();
}

Command CommandQueue::AggressivePrecharge() {
    for(auto i = 0; i < config_.ranks; i++) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto j = 0; j < config_.bankgroups; j++) {
                if(channel_state_.IsRowOpen(i, j, k)) {
                    auto cmd = Command(CommandType::PRECHARGE, -1, i, j, k, -1);
                    if(channel_state_.IsReady(cmd, clk)) {
                        bool pending_row_hits_exist = false;
                        auto open_row = channel_state_.OpenRow(i, j, k);
                        auto& queue = GetQueue(i, j, k);
                        for(auto pending_req : queue) {
                            if( pending_req->cmd_.row_ == open_row) { //ToDo - And same bank if queues are rank based
                                pending_row_hits_exist = true;
                                break;
                            }
                        }
                        if(!pending_row_hits_exist)
                            return Command(CommandType::PRECHARGE, -1, i, j, k, -1);
                    }
                }
            }
        }
    }
    return Command();
}

bool CommandQueue::InsertReq(Request* req) {
    auto r = req->cmd_.rank_, bg = req->cmd_.bankgroup_, b = req->cmd_.bank_;
    if( req_q_[r][bg][b].size() < config_.queue_size ) {
        req_q_[r][bg][b].push_back(req);
        return true;
    }
    else
        return false;
}

inline void CommandQueue::IterateNext() {
    next_bankgroup_ = (next_bankgroup_ + 1) % config_.bankgroups;
    if(next_bankgroup_ == 0) {
        next_bank_ = (next_bank_ + 1) % config_.banks_per_group;
        if(next_bank_ == 0) {
            next_rank_ = (next_rank_ + 1) % config_.ranks;
        }
    }
    return;
}
