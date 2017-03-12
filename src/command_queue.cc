#include "command_queue.h"
#include <vector>
#include <list>

using namespace std;

CommandQueue::CommandQueue(int ranks, int bankgroups, int banks_per_group, const ChannelState& channel_state) :
    clk(0),
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    next_rank_(0),
    next_bankgroup_(0),
    next_bank_(0),
    size_q_(16),
    req_q_(ranks, vector< vector<list<Request*>> >(bankgroups, vector<list<Request*>>(banks_per_group, list<Request*>()) ) ),
    channel_state_(channel_state)
{

}

Command CommandQueue::GetCommandToIssue() {

    //Unified Per bank queues

    //Rank, Bank, Bankgroup traversal of queues
    for(auto i = 0; i < ranks_; i++) {
        for(auto k = 0; k < banks_per_group_; k++) {
            for(auto j = 0; j < bankgroups_; j++) {
                if( !channel_state_.IsRefreshWaiting(next_rank_, next_bankgroup_, next_bank_) ) {
                    auto queue = GetQueue(next_rank_, next_bankgroup_, next_bank_);
                    //Prioritize row hits while honoring read, write dependencies
                    for(auto req_itr = queue.begin(); req_itr != queue.end(); req_itr++) {
                        auto req = *req_itr;
                        Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
                        if ( req->cmd_.cmd_type_ == cmd.cmd_type_) { //Essentially checking for a row hit
                            if(channel_state_.IsReady(cmd, clk)) {
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
                        }
                    }
                    //No read/write to issue.
                    for(auto itr = queue.begin(); itr != queue.end(); itr++) {
                        auto req = *itr;
                        Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
                        if( !cmd.IsReadWrite() && channel_state_.IsReady(cmd, clk)) {
                            if(cmd.cmd_type_ == CommandType::PRECHARGE) {
                                // Only attempt issuing a precharge if either one of the following conditions are satisfied
                                // 1. There are no pending row hits to the open row in the bank
                                // 2. There are pending row hits to the open row but the max allowed cap for row hits has been exceeded
                                bool pending_row_hits_exist = false;
                                auto open_row = channel_state_.OpenRow(cmd.rank_, cmd.bankgroup_, cmd.bank_);
                                for(auto pending_req : queue) {
                                    if( pending_req->cmd_.row_ == open_row) {
                                        pending_row_hits_exist = true;
                                        break;
                                    }
                                }
                                if(pending_row_hits_exist && channel_state_.RowHitCount(cmd.rank_, cmd.bankgroup_, cmd.bank_) < 4)
                                    break;
                            }
                            return cmd;
                        }
                    }
                }
                IterateNext();
            }
        }
    }
    return Command();

    //Unified per rank queues

    //Unified single queue

    //Separate read/write queues with write bufer

    //Separate read/write queues per rank with write buffer

    //Separate read/write queues per bank with write buffer
}

bool CommandQueue::InsertReq(Request* req) {
    auto r = req->cmd_.rank_, bg = req->cmd_.bankgroup_, b = req->cmd_.bank_;
    if( req_q_[r][bg][b].size() < size_q_ ) {
        req_q_[r][bg][b].push_back(req);
        return true;
    }
    else
        return false;
}

inline void CommandQueue::IterateNext() {
    next_bankgroup_ = (next_bankgroup_ + 1) % bankgroups_;
    if(next_bankgroup_ == 0) {
        next_bank_ = (next_bank_ + 1) % banks_per_group_;
        if(next_bank_ == 0) {
            next_rank_ = (next_rank_ + 1) % ranks_;
        }
    }
    return;
}


list<Request*>& CommandQueue::GetQueue(int rank, int bankgroup, int bank) {
    return req_q_[rank][bankgroup][bank];
}