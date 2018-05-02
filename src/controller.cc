#include "controller.h"
#include <iostream>

namespace dramsim3 {

#ifdef THERMAL
Controller::Controller(int channel, const Config &config, const Timing &timing,
                       Statistics &stats, ThermalCalculator *thermalcalc,
                       std::function<void(uint64_t)> read_callback,
                       std::function<void(uint64_t)> write_callback)
#else
Controller::Controller(int channel, const Config &config, const Timing &timing,
                       Statistics &stats,
                       std::function<void(uint64_t)> read_callback,
                       std::function<void(uint64_t)> write_callback)
#endif  // THERMAL
    : read_callback_(read_callback),
      write_callback_(write_callback),
      channel_id_(channel),
      clk_(0),
      config_(config),
#ifdef THERMAL
      channel_state_(config, timing, stats, thermalcalc),
#else
      channel_state_(config, timing, stats),
#endif  // THERMAL
      cmd_queue_(channel_id_, config, channel_state_, stats),
      refresh_(channel_id_, config, channel_state_, cmd_queue_, stats),
      stats_(stats) {
    transaction_q_.reserve(config_.trans_queue_size);
}

void Controller::ClockTick() {
    // Return completed transactions back to the CPU
    for (auto it = return_queue_.begin(); it != return_queue_.end();) {
        if (clk_ >= it->complete_cycle) {
            stats_.access_latency.AddValue(clk_ - it->added_cycle);
            if (it->is_write) {
                stats_.numb_read_reqs_issued++;
                write_callback_(it->addr);
            } else {
                stats_.numb_write_reqs_issued++;
                read_callback_(it->addr);
            }
            it = return_queue_.erase(it);
        } else {
            ++it;
        }
    }

    // add background power, we have to cram all ranks and banks together now...
    // if we have rank states it would make life easier
    for (int i = 0; i < config_.ranks; i++) {
        if (channel_state_.IsRankSelfRefreshing(i)) {
            // stats_.sref_energy[channel_id_][i]++;
            stats_.sref_cycles[channel_id_][i]++;
        } else {
            bool all_idle = channel_state_.IsAllBankIdleInRank(i);
            if (all_idle) {
                stats_.all_bank_idle_cycles[channel_id_][i]++;
                // stats_.pre_stb_energy[channel_id_][i]++;
            } else {
                stats_.active_cycles[channel_id_][i]++;
                // stats_.act_stb_energy[channel_id_][i]++;
            }
        }
    }

    // Move idle ranks into self-refresh mode to save power
    if (config_.enable_self_refresh) {
        for (auto i = 0; i < config_.ranks; i++) {
            // update rank idleness
            if (cmd_queue_.rank_queues_empty[i] &&
                clk_ - cmd_queue_.rank_queues_empty_from_time_[i] >=
                    static_cast<uint64_t>(
                        config_.idle_cycles_for_self_refresh) &&
                !channel_state_.rank_in_self_refresh_mode_[i]) {
                auto addr = Address();
                addr.channel = channel_id_;
                addr.rank = i;
                auto self_refresh_enter_cmd =
                    Command(CommandType::SELF_REFRESH_ENTER, addr, -1);
                auto cmd =
                    channel_state_.GetRequiredCommand(self_refresh_enter_cmd);
                if (channel_state_.IsReady(cmd, clk_))
                    if (cmd.cmd_type == CommandType::SELF_REFRESH_ENTER) {
                        // clear refresh requests from the queue for the rank
                        // that is about to go into self-refresh mode
                        for (auto refresh_req_iter =
                                 refresh_.refresh_q_.begin();
                             refresh_req_iter != refresh_.refresh_q_.end();
                             refresh_req_iter++) {
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

    // Refresh command is queued
    refresh_.ClockTick();
    if (!refresh_.refresh_q_.empty()) {
        auto refresh_itr = refresh_.refresh_q_.begin();
        if (channel_state_.need_to_update_refresh_waiting_status_) {
            channel_state_.need_to_update_refresh_waiting_status_ = false;
            channel_state_.UpdateRefreshWaitingStatus((**refresh_itr), true);
        }
        auto cmd = refresh_.GetRefreshOrAssociatedCommand(refresh_itr);
        if (cmd.IsValid()) {
            channel_state_.IssueCommand(cmd, clk_);
            if (cmd.IsRefresh()) {
                channel_state_.need_to_update_refresh_waiting_status_ = true;
                channel_state_.UpdateRefreshWaitingStatus(
                    cmd, false);  // TODO - Move this to channelstate update?
            }
            return;  // TODO - What about HBM dual command issue?
        }
    }

    auto cmd = cmd_queue_.GetCommandToIssue();
    if (cmd.IsValid()) {
        // if read/write, update pending queue and return queue
        if (cmd.IsReadWrite()) {
            auto it = pending_trans_.find(cmd.id);
            if (cmd.IsRead()) {
                it->second.complete_cycle = clk_ + config_.read_delay;
            } else {
                it->second.complete_cycle = clk_ + config_.write_delay;
            }
            // std::cout << "Command " << cmd.id << " finished! " << std::endl;
            // std::cout << it->first << it->second << std::endl;
            // std::cout << "Trans " << it->second.addr << " moved to return q,"
            //           << "to be exit at " << std::setw(6) 
            //           << it->second.complete_cycle << std::endl;
            return_queue_.push_back(it->second);
            pending_trans_.erase(it);
        }

        channel_state_.IssueCommand(cmd, clk_);

        if (config_.enable_hbm_dual_cmd) {  // TODO - Current implementation
                                            // doesn't do dual command issue
                                            // during refresh
            auto second_cmd = cmd_queue_.GetCommandToIssue();
            if (second_cmd.IsValid()) {
                if (cmd.IsReadWrite() ^ second_cmd.IsReadWrite()) {
                    channel_state_.IssueCommand(second_cmd, clk_);
                    stats_.hbm_dual_command_issue_cycles++;
                }
                if (!cmd.IsReadWrite() && !second_cmd.IsReadWrite()) {
                    stats_.hbm_dual_non_rw_cmd_attempt_cycles++;
                }
            }
        }
        return;
    } else if (config_.aggressive_precharging_enabled) {
        // Look for closing open banks if any. (Aggressive precharing)
        // To which no read/write requests exist in the queues
        auto pre_cmd = cmd_queue_.AggressivePrecharge();
        if (pre_cmd.IsValid()) {
            channel_state_.IssueCommand(pre_cmd, clk_);
        }
    }

    // put command into command queue, only lookup for a certain amount
    const int max_lookup_depth = 16;
    int lookup_depth = 0;
    for (auto it = transaction_q_.begin(); it != transaction_q_.end();) {
        if (lookup_depth >= max_lookup_depth) {
            break;  // stop wasting time
        }
        auto cmd = TransToCommand(*it);
        if (cmd_queue_.WillAcceptCommand(cmd.Rank(), cmd.Bankgroup(),
                                         cmd.Bank())) {
            cmd_queue_.AddCommand(cmd);
            // std::cout << "Trans " << it->addr << " moved to pending Q" << std::endl;
            pending_trans_.insert(std::make_pair(cmd_id_, *it));
            it = transaction_q_.erase(it);
            cmd_id_ ++;
            // reset cmd_id_ to avoid overflow, there will not be 32k cmds
            // in fly so we should be fine
            if (cmd_id_ == 32768) {
                cmd_id_ = 0;
            }
            // only allow one transaction scheduled per cycle
            break;
        } else {
            ++it;
        }
    }

    // std::cout << "Queue Depts " << " at " << clk_ << ": " << std::endl;
    // std::cout << "TransQ:  " << transaction_q_.size() << std::endl;
    // for (auto trans:transaction_q_) {
    //     std::cout << trans << std::endl;
    // }
    // std::cout << "PendingQ:" << pending_trans_.size() << std::endl;
    // for (auto it:pending_trans_) {
    //     std::cout << std::setw(5) << it.first;
    //     std::cout << it.second <<std::endl;
    // }
    // std::cout << "ReturnQ: " << return_queue_.size() << std::endl;
    // for (auto trans:return_queue_) {
    //     std::cout << trans << std::endl;
    // }
    clk_++;
    cmd_queue_.clk_++;
}

bool Controller::WillAcceptTransaction() {
    return transaction_q_.size() < transaction_q_.capacity();
}

bool Controller::AddTransaction(Transaction trans) {
    trans.added_cycle = clk_;
    transaction_q_.push_back(trans);
    return transaction_q_.size() <= transaction_q_.capacity();
}

Command Controller::TransToCommand(const Transaction &trans) {
    auto addr = AddressMapping(trans.addr);
    CommandType cmd_type;
    if (scheduling_policy_ == SchedulingPolicy::OPEN_PAGE) {
        cmd_type = trans.is_write ? CommandType::WRITE : CommandType::READ;
    } else {
        cmd_type = trans.is_write ? CommandType::WRITE_PRECHARGE
                                  : CommandType::READ_PRECHARGE;
    }
    Command cmd = Command(cmd_type, addr, cmd_id_);
    return cmd;
}

int Controller::QueueUsage() const { return cmd_queue_.QueueUsage(); }

}  // namespace dramsim3
