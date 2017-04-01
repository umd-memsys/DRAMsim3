#include <iostream>
#include "controller.h"
#include "statistics.h"

using namespace std;

Controller::Controller(int channel, const Config &config, const Timing &timing, Statistics &stats) :
    channel_(channel),
    clk_(0),
    channel_state_(config, timing),
    cmd_queue_(config, channel_state_, stats), //TODO - Isn't it really a request_queue. Why call it command_queue?
    refresh_(config, channel_state_, cmd_queue_)
{}

void Controller::ClockTick() {
    clk_++;
    cmd_queue_.clk_++;
    //Refresh command is queued
    refresh_.ClockTick();
    if( !refresh_.refresh_q_.empty()) {
        auto refresh_itr = refresh_.refresh_q_.begin(); //TODO - Or chose which refresh request in the queue to prioritize to execute
        channel_state_.UpdateRefreshWaitingStatus((*refresh_itr)->cmd_, true); //TODO - Why is this updated each time? Not smart.
        auto cmd = refresh_.GetRefreshOrAssociatedCommand(refresh_itr);
        if(cmd.IsValid()) {
            channel_state_.IssueCommand(cmd, clk_);
            if(cmd.IsRefresh())
                channel_state_.UpdateRefreshWaitingStatus(cmd, false);
            return;
        }
    }
    //Read write queues
    auto cmd = cmd_queue_.GetCommandToIssue();
    if(cmd.IsValid()) {
        channel_state_.IssueCommand(cmd, clk_);
    }
    else {
        //Look for closing open banks if any. (Aggressive precharing)
        //To which no read/write requests exist in the queues
        auto pre_cmd = cmd_queue_.AggressivePrecharge();
        if(pre_cmd.IsValid()) {
            channel_state_.IssueCommand(pre_cmd, clk_);
        }
    }
}

bool Controller::InsertReq(Request* req) {
    return cmd_queue_.InsertReq(req);
}

