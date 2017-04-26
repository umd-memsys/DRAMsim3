#include <iostream>
#include "controller.h"

using namespace std;
using namespace dramcore;

Controller::Controller(int channel, const Config &config, const Timing &timing, Statistics &stats, std::function<void(uint64_t)>& callback) :
    callback_(callback),
    channel_(channel),
    clk_(0),
    channel_state_(config, timing, stats),
    cmd_queue_(config, channel_state_, stats, callback_), //TODO - Isn't it really a request_queue. Why call it command_queue?
    refresh_(config, channel_state_, cmd_queue_, stats)
{
    if (!config.validation_output_file.empty()) {
        cout << "Validation Command Trace write to "<< config.validation_output_file << endl;
        val_output_enable = true;
        val_output_.open(config.validation_output_file, std::ofstream::out);
    }
}

void Controller::ClockTick() {
    clk_++;
    cmd_queue_.clk_++;

    //Return already issued read requests back to the CPU
    for( auto req_itr = cmd_queue_.issued_req_.begin(); req_itr !=  cmd_queue_.issued_req_.end(); req_itr++) {
        auto issued_req = *req_itr;
        if(clk_ > issued_req->exit_time_) {
            //Return request to cpu
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
            if(cmd.IsRefresh())
                channel_state_.UpdateRefreshWaitingStatus(cmd, false);
            return;
        }
    }
    //Read write queues
    auto cmd = cmd_queue_.GetCommandToIssue();
    if(cmd.IsValid()) {
        channel_state_.IssueCommand(cmd, clk_);
        #ifdef DEBUG_OUTPUT
            cout << left << setw(8) << clk_ << " " << cmd <<std::endl;
        #endif  //DEBUG_OUTPUT
        if (val_output_enable) {
            val_output_ << left << setw(8) << clk_ << " " << cmd <<std::endl;
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

Controller::~Controller() {
    val_output_.close();
}

// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C"
{
  void libdramcore_is_present(void)
  {
	;
  }
}
