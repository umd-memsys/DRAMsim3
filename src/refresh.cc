#include "refresh.h"
#include <iostream>

using namespace std;

Refresh::Refresh(int ranks, int bankgroups, int banks_per_group, const ChannelState& channel_state) :
    clk(0),
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    channel_state_(channel_state),
    last_bank_refresh_(ranks, vector< vector<long>>(bankgroups, vector<long>(banks_per_group, 0))),
    last_rank_refresh_(ranks, 0),
    next_rank_(0)
{
    printf("Creating refresh object with bankgroups = %d, banks = %d\n", bankgroups_, banks_per_group_);
}

void Refresh::ClockTick() {
    clk++;
    InsertRefresh();
    return;
}

void Refresh::InsertRefresh() {
    if( clk % tREFI == 0) {
        refresh_q_.push_back(new Request(CommandType::REFRESH, next_rank_));
        IterateNext();
    }
    return;
}

Command Refresh::GetRefreshOrAssociatedCommand(list<Request*>::iterator req_itr) {
    //Issue row buffer hits upto cap (Easy Peesy to implement)
    //Issue only stream of row buffer hits (nothing out or order) upto a cap (DRAMSim2)

    //Dumbly issue refresh without caring about pending accesses
    auto req = *req_itr;
    auto cmd = channel_state_.GetRequiredCommand(req->cmd_);
    if(channel_state_.IsReady(cmd, clk)) {
        if(req->cmd_.cmd_type_ == cmd.cmd_type_) {
            //Sought of actually issuing the refresh command
            
            delete(*req_itr);
            refresh_q_.erase(req_itr);
        }
        return cmd;
    }
    return Command();
}

inline void Refresh::IterateNext() {
    next_rank_ = (next_rank_ + 1) % ranks_;
    return;
}