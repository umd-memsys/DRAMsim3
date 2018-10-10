#include "controller.h"
#include <iostream>

namespace dramsim3 {

#ifdef THERMAL
Controller::Controller(int channel, const Config &config, const Timing &timing,
                       Statistics &stats, ThermalCalculator &thermal_calc,
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
      channel_state_(config, timing, stats),
      cmd_queue_(channel_id_, config, channel_state_, stats),
      refresh_(config, channel_state_, cmd_queue_, stats),
      stats_(stats),
#ifdef THERMAL
      thermal_calc_(thermal_calc),
#endif  // THERMAL
      cmd_id_(0),
      max_cmd_id_(config_.cmd_queue_size * config_.banks * 2),
      row_buf_policy_(config.row_buf_policy == "CLOSE_PAGE"
                          ? RowBufPolicy::CLOSE_PAGE
                          : RowBufPolicy::OPEN_PAGE) {
    transaction_queue_.reserve(config_.trans_queue_size);
#ifdef GENERATE_TRACE
    std::string trace_file_name = config_.output_prefix + "cmd.trace";
    RenameFileWithNumber(trace_file_name, channel_id);
    std::cout << "Command Trace write to " << trace_file_name << std::endl;
    cmd_trace_.open(trace_file_name, std::ofstream::out);
#endif  // GENERATE_TRACE
}

void Controller::ClockTick() {
    // Return completed transactions back to the CPU
    for (auto it = return_queue_.begin(); it != return_queue_.end();) {
        if (clk_ >= it->complete_cycle) {
            stats_.access_latency.AddValue(clk_ - it->added_cycle);
            if (it->is_write) {
                stats_.numb_write_reqs_issued++;
                write_callback_(it->addr);
            } else {
                stats_.numb_read_reqs_issued++;
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
            } else {
                stats_.active_cycles[channel_id_][i]++;
            }
        }
    }

    // currently there can be only 1 command at the bus at the same time
    bool command_issued = false;
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
                            if (refresh_req.Rank() == cmd.Rank()) {
                                refresh_.refresh_q_.erase(refresh_req_iter);
                            }
                        }
                    }
                command_issued = true;
                IssueCommand(cmd);
            }
        }
    }

    if (refresh_.IsRefWaiting()) {
        auto cmd = refresh_.GetRefreshOrAssociatedCommand();
        if (cmd.IsValid() && (!command_issued)) {
            IssueCommand(cmd);
            command_issued = true;
        }
    }
    refresh_.ClockTick();

    if ((!command_issued) && (!refresh_.IsRefWaiting())) {
        auto cmd = cmd_queue_.GetCommandToIssue();
        if (cmd.IsValid()) {
            IssueCommand(cmd);
            command_issued = true;

            if (config_.enable_hbm_dual_cmd) {
                auto second_cmd = cmd_queue_.GetCommandToIssue();
                if (second_cmd.IsValid()) {
                    if (second_cmd.IsReadWrite() != cmd.IsReadWrite()) {
                        IssueCommand(cmd);
                        stats_.hbm_dual_command_issue_cycles++;
                    } else {
                        stats_.hbm_dual_non_rw_cmd_attempt_cycles++;
                    }
                }
            }
        }
    }

    // look ahead this amount of transactions to put in cmd queue
    // considering hardware cost to implement this... it won't be a large number
    const int max_lookup_depth = 4;
    int lookup_depth = 0;
    for (auto it = transaction_queue_.begin();
         it != transaction_queue_.end();) {
        if (lookup_depth >= max_lookup_depth) {
            break;  // stop wasting time
        }
        lookup_depth++;
        auto cmd = TransToCommand(*it);
        if (cmd_queue_.WillAcceptCommand(cmd.Rank(), cmd.Bankgroup(),
                                         cmd.Bank())) {
            cmd_queue_.AddCommand(cmd);
            pending_queue_.insert(std::make_pair(cmd_id_, *it));
            it = transaction_queue_.erase(it);
            // Reset cmd_id
            cmd_id_++;
            cmd_id_ = cmd_id_ % max_cmd_id_;
            break;  // only allow one transaction scheduled per cycle
        } else {
            ++it;
        }
    }
    clk_++;
    cmd_queue_.clk_++;
    return;
}

bool Controller::WillAcceptTransaction() const {
    return transaction_queue_.size() < transaction_queue_.capacity();
}

bool Controller::AddTransaction(Transaction trans) {
    trans.added_cycle = clk_;
    transaction_queue_.push_back(trans);
    return transaction_queue_.size() <= transaction_queue_.capacity();
}

void Controller::IssueCommand(const Command& tmp_cmd) {
    Command cmd = Command(tmp_cmd.cmd_type, tmp_cmd.addr, tmp_cmd.id);
#ifdef DEBUG_OUTPUT
    std::cout << std::left << std::setw(8) << clk << " " << cmd << std::endl;
#endif  // DEBUG_OUTPUT
#ifdef GENERATE_TRACE
    cmd_trace_ << std::left << std::setw(18) << clk << " " << cmd << endl;
#endif  // GENERATE_TRACE
#ifdef THERMAL
    // add channel in, only needed by thermal module
    cmd.addr.channel = channel_id_;
    thermal_calc_.UpdatePower(cmd, clk_);
#endif  // THERMAL
    // if read/write, update pending queue and return queue
    if (cmd.IsReadWrite()) {
        ProcessRWCommand(cmd);
    } else if (cmd.IsRefresh()) {
        refresh_.RefreshIssued(cmd);
    }
    channel_state_.UpdateTimingAndStates(cmd, clk_);
}

void Controller::ProcessRWCommand(const Command& cmd) {
    cmd_queue_.IssueRWCommand(cmd);
    auto it = pending_queue_.find(cmd.id);
    if (cmd.IsRead()) {
        it->second.complete_cycle = clk_ + config_.read_delay;
    } else {
        it->second.complete_cycle = clk_ + config_.write_delay;
    }
    return_queue_.push_back(it->second);
    pending_queue_.erase(it);
    return;
}

Command Controller::TransToCommand(const Transaction &trans) {
    auto addr = AddressMapping(trans.addr);
    CommandType cmd_type;
    if (row_buf_policy_ == RowBufPolicy::OPEN_PAGE) {
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
