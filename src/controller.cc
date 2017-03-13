#include <iostream>
#include "controller.h"

using namespace std;

Controller::Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing) :
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    clk(0),
    channel_state_(ranks, bankgroups, banks_per_group, timing),
    cmd_queue_(ranks, bankgroups, banks_per_group, channel_state_),
    refresh_(ranks, bankgroups, banks_per_group, channel_state_, cmd_queue_)
{
    printf("Creating a controller object with ranks = %d, bankgroups = %d, banks_per_group = %d\n", ranks_, bankgroups_, banks_per_group_);
}

void Controller::ClockTick() {
    //Refresh command is queued
    refresh_.ClockTick();
    if( !refresh_.refresh_q_.empty()) {
        auto req_itr = refresh_.refresh_q_.begin();
        channel_state_.UpdateRefreshWaitingStatus((*req_itr)->cmd_, true); //Why is this updated each time? Not smart.
        auto cmd = refresh_.GetRefreshOrAssociatedCommand(req_itr);
        if(cmd.IsValid()) {
            channel_state_.IssueCommand(cmd, clk);
            if(cmd.IsRefresh())
                channel_state_.UpdateRefreshWaitingStatus(cmd, false);
            return;
        }
    }

    //Read write queues
    auto cmd = cmd_queue_.GetCommandToIssue();
    if(cmd.IsValid()) {
        channel_state_.IssueCommand(cmd, clk);
    }
    else {
        //Look for closing open banks if any. (Aggressive precharing)
        auto pre_cmd = AggressivePrecharge();
        if(cmd.IsValid()) {
            channel_state_.IssueCommand(cmd, clk);
        }
    }
    clk++;
    cmd_queue_.clk++;
}





bool Controller::InsertReq(Request* req) {
    return cmd_queue_.InsertReq(req);
}

