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
    //Rank, Bank, Bankgroup traversal of queues
    for(auto i = 0; i < ranks_; i++) {
        for(auto k = 0; k < banks_per_group_; k++) {
            for(auto j = 0; j < bankgroups_; j++) {
                if( !channel_state_.IsRefreshWaiting(next_rank_, next_bankgroup_, next_bank_) ) {
                    list<Request*>& queue = req_q_[next_rank_][next_bankgroup_][next_bank_];
                    //FRFCFS (First ready first come first serve)
                    //Search the queue to pickup the first request whose required command could be issued this cycle
                    for(auto itr = queue.begin(); itr != queue.end(); itr++) {
                        auto req = *itr;
                        Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
                        if(channel_state_.IsReady(cmd, clk)) {
                            if(req->cmd_.cmd_type_ == cmd.cmd_type_) {
                                //Sought of actually issuing the read/write command
                                delete(*itr);
                                queue.erase(itr);
                            }
                            return cmd;
                        }
                        //TODO - PreventRead write dependencies
                        //Having unified read write queues appears to a very dumb idea!
                    }
                }
                IterateNext();
            }
        }
    }
    return Command();
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