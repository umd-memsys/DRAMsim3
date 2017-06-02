#include <iostream>
#include "controller.h"

using namespace std;
using namespace dramcore;

Controller::Controller(int channel, const Config &config, const Timing &timing, Statistics &stats, std::function<void(uint64_t)>& callback) :
    callback_(callback),
    channel_id_(channel),
    clk_(0),
    config_(config),
    channel_state_(config, timing, stats),
    cmd_queue_(config, channel_state_, stats, callback_), //TODO - Isn't it really a request_queue. Why call it command_queue?
    refresh_(channel_id_, config, channel_state_, cmd_queue_, stats),
    stats_(stats)
{
}

Controller::~Controller() {
    val_output_.close(); //TODO - Where is this opened? Why is this closed here
}

void Controller::ClockTick() {
    clk_++;
    cmd_queue_.clk_++;

    //Return already issued read requests back to the CPU
    for( auto req_itr = cmd_queue_.issued_req_.begin(); req_itr !=  cmd_queue_.issued_req_.end(); req_itr++) {
        auto issued_req = *req_itr;
        if(clk_ > issued_req->exit_time_) {
            //Return request to cpu
            stats_.access_latency.AddValue(clk_ - issued_req->arrival_time_);
            callback_(issued_req->hex_addr_);
            delete(issued_req);
            cmd_queue_.issued_req_.erase(req_itr);
            break; // Returning one request per cycle
        }
    }

    //Refresh command is queued
    refresh_.ClockTick();
    if( !refresh_.refresh_q_.empty()) {
        auto refresh_itr = refresh_.refresh_q_.begin(); //TODO - Or chose which refresh request in the queue to prioritize to execute
        channel_state_.UpdateRefreshWaitingStatus((*refresh_itr)->cmd_, true); //TODO - Why is this updated each time? Not smart.
        auto cmd = refresh_.GetRefreshOrAssociatedCommand(refresh_itr);
        if(cmd.IsValid()) {
            channel_state_.IssueCommand(cmd, clk_);
            if(cmd.IsRefresh()) {
                channel_state_.UpdateRefreshWaitingStatus(cmd, false);
            }
            return;
        }
    }

    auto cmd = cmd_queue_.GetCommandToIssue();
    if(cmd.IsValid()) {
        channel_state_.IssueCommand(cmd, clk_);
        
        if (config_.IsHBM()){
            auto second_cmd = cmd_queue_.GetCommandToIssue();
            if (second_cmd.IsValid()) {
                if (cmd.IsReadWrite() ^ second_cmd.IsReadWrite()) {
                    channel_state_.IssueCommand(second_cmd, clk_);
                    stats_.hbm_dual_command_issue_cycles++;
                }
                if(!cmd.IsReadWrite() && !second_cmd.IsReadWrite()) {
                    stats_.hbm_dual_non_rw_cmd_attempt_cycles++;
                }
            }
        }
    }

    /* //TODO Make- Aggressive precharing a knob
    else {
        //Look for closing open banks if any. (Aggressive precharing)
        //To which no read/write requests exist in the queues
        auto pre_cmd = cmd_queue_.AggressivePrecharge();
        if(pre_cmd.IsValid()) {
            channel_state_.IssueCommand(pre_cmd, clk_);
        }
    }
    */
}

bool Controller::InsertReq(Request* req) {
    return cmd_queue_.InsertReq(req);
}
