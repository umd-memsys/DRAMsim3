#include <iostream>
#include "controller.h"

using namespace std;
using namespace dramcore;

Controller::Controller(int channel, const Config &config, const Timing &timing, Statistics &stats, ThermalCalculator *thermcalc, std::function<void(uint64_t)>& callback) :
    callback_(callback),
    channel_id_(channel),
    clk_(0),
    config_(config),
    channel_state_(config, timing, stats, thermcalc),
    cmd_queue_(channel_id_, config, channel_state_, stats, callback_), //TODO - Isn't it really a request_queue. Why call it command_queue?
    refresh_(channel_id_, config, channel_state_, cmd_queue_, stats),
    stats_(stats)
{
}


void Controller::ClockTick() {
    clk_++;
    cmd_queue_.clk_++;

    //Return already issued read/write requests back to the CPU
    for( auto req_itr = cmd_queue_.issued_req_.begin(); req_itr !=  cmd_queue_.issued_req_.end(); req_itr++) {
        auto issued_req = *req_itr;
        if(clk_ > issued_req->exit_time_) {
            //Return request to cpu
            stats_.access_latency.AddValue(clk_ - issued_req->arrival_time_);
            callback_(issued_req->hex_addr_);
            delete(issued_req);
            cmd_queue_.issued_req_.erase(req_itr++);
            break; // Returning one request per cycle. TODO - Make this a knob?
        }
    }

    // add background power, we have to cram all ranks and banks together now...
    // if we have rank states it would make life easier
    for (unsigned i = 0; i < config_.ranks; i++) {
        if (channel_state_.IsRankSelfRefreshing(i)) {
            stats_.sref_energy++;
        } else {
            bool all_idle = channel_state_.IsAllBankIdleInRank(i);
            if (all_idle) {
#ifdef DEBUG_POWER
                stats_.all_bank_idle_cycles++;
#endif // DEBUG_POWER
                stats_.pre_stb_energy++;
            } else {
#ifdef DEBUG_POWER
                stats_.active_cycles++;
#endif // DEBUG_POWER
                stats_.act_stb_energy++;
            }
        }
    }

    //Move idle ranks into self-refresh mode to save power
    if(config_.enable_self_refresh) {
        for (auto i = 0; i < config_.ranks; i++) {
            //update rank idleness
            if (cmd_queue_.rank_queues_empty[i] &&
                clk_ - cmd_queue_.rank_queues_empty_from_time_[i] >= config_.idle_cycles_for_self_refresh &&
                !channel_state_.rank_in_self_refresh_mode_[i]) {
                auto addr = Address();
                addr.channel_ = channel_id_;
                addr.rank_ = i;
                auto self_refresh_enter_cmd = Command(CommandType::SELF_REFRESH_ENTER, addr);
                auto cmd = channel_state_.GetRequiredCommand(self_refresh_enter_cmd);
                if (channel_state_.IsReady(cmd, clk_))
                    if (cmd.cmd_type_ == CommandType::SELF_REFRESH_ENTER) {
                        //clear refresh requests from the queue for the rank that is about to go into self-refresh mode
                        for (auto refresh_req_iter = refresh_.refresh_q_.begin();
                             refresh_req_iter != refresh_.refresh_q_.end(); refresh_req_iter++) {
                            auto refresh_req = *refresh_req_iter;
                            if (refresh_req->Rank() == cmd.Rank()) {
                                delete (refresh_req);
                                refresh_.refresh_q_.erase(refresh_req_iter);
                            }
                        }
                    }
                channel_state_.IssueCommand(cmd, clk_);
            }
        }
    }

    //Refresh command is queued
    refresh_.ClockTick();
    if( !refresh_.refresh_q_.empty()) {
        auto refresh_itr = refresh_.refresh_q_.begin();
        if(channel_state_.need_to_update_refresh_waiting_status_) {
            channel_state_.need_to_update_refresh_waiting_status_ = false;
            channel_state_.UpdateRefreshWaitingStatus((*refresh_itr)->cmd_, true);
        }
        auto cmd = refresh_.GetRefreshOrAssociatedCommand(refresh_itr);
        if(cmd.IsValid()) { 
            channel_state_.IssueCommand(cmd, clk_);
            if(cmd.IsRefresh()) {
                channel_state_.need_to_update_refresh_waiting_status_ = true;
                channel_state_.UpdateRefreshWaitingStatus(cmd, false); //TODO - Move this to channelstate update?
            }
            return; //TODO - What about HBM dual command issue?
        }
    }

    auto cmd = cmd_queue_.GetCommandToIssue();
    if(cmd.IsValid()) {
        channel_state_.IssueCommand(cmd, clk_);
        
        if (config_.IsHBM()){ //TODO - Current implementation doesn't do dual command issue during refresh
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
        return;
    }
    else if(config_.aggressive_precharging_enabled) {
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
