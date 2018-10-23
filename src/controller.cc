#include "controller.h"
#include <iostream>
#include <limits>

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
      refresh_(config, channel_state_),
      stats_(stats),
#ifdef THERMAL
      thermal_calc_(thermal_calc),
#endif  // THERMAL
      cmd_id_(0),
      row_buf_policy_(config.row_buf_policy == "CLOSE_PAGE"
                          ? RowBufPolicy::CLOSE_PAGE
                          : RowBufPolicy::OPEN_PAGE),
      write_draining_(0) {
    read_queue_.reserve(config_.trans_queue_size);
    write_queue_.reserve(config_.trans_queue_size);
#ifdef GENERATE_TRACE
    std::string trace_file_name = config_.output_prefix + "cmd.trace";
    RenameFileWithNumber(trace_file_name, channel_id);
    std::cout << "Command Trace write to " << trace_file_name << std::endl;
    cmd_trace_.open(trace_file_name, std::ofstream::out);
#endif  // GENERATE_TRACE
}

void Controller::ClockTick() {
    // Return completed read transactions back to the CPU
    for (auto it = return_queue_.begin(); it != return_queue_.end();) {
        if (clk_ >= it->complete_cycle) {
            stats_.access_latency.AddValue(clk_ - it->added_cycle);
            if (it->is_write) {
                std::cerr << "cmd id overflow!" << std::endl;
                exit(1);
            } else {
                stats_.num_reads_done++;
                read_callback_(it->addr);
                it = return_queue_.erase(it);
            }
        } else {
            ++it;
        }
    }

    // update refresh counter
    refresh_.ClockTick();

    bool cmd_issued = false;
    auto cmd = cmd_queue_.GetCommandToIssue();
    if (cmd.IsValid()) {
        IssueCommand(cmd);
        cmd_issued = true;

        if (config_.enable_hbm_dual_cmd) {
            auto second_cmd = cmd_queue_.GetCommandToIssue();
            if (second_cmd.IsValid()) {
                if (second_cmd.IsReadWrite() != cmd.IsReadWrite()) {
                    IssueCommand(second_cmd);
                    stats_.hbm_dual_cmds++;
                }
            }
        }
    }

    // power updates pt 1
    for (int i = 0; i < config_.ranks; i++) {
        if (channel_state_.IsRankSelfRefreshing(i)) {
            // stats_.sref_energy[channel_id_][i]++;
            stats_.sref_cycles[channel_id_][i]++;
        } else {
            bool all_idle = channel_state_.IsAllBankIdleInRank(i);
            if (all_idle) {
                stats_.all_bank_idle_cycles[channel_id_][i]++;
                channel_state_.rank_idle_cycles[i] += 1;
            } else {
                stats_.active_cycles[channel_id_][i]++;
                // reset
                channel_state_.rank_idle_cycles[i] = 0;
            }
        }
    }

    // power updates pt 2: move idle ranks into self-refresh mode to save power
    if (config_.enable_self_refresh && !cmd_issued) {
        for (auto i = 0; i < config_.ranks; i++) {
            if (channel_state_.IsRankSelfRefreshing(i)) {
                // wake up!
                if (!cmd_queue_.rank_q_empty[i]) {
                    auto addr = Address();
                    addr.rank = i;
                    auto cmd = Command(CommandType::SREF_EXIT, addr, -1);
                    if (channel_state_.IsReady(cmd, clk_)){
                        IssueCommand(cmd);
                        break;
                    }
                }
            } else {
                if (cmd_queue_.rank_q_empty[i] &&
                    channel_state_.rank_idle_cycles[i] >=
                        config_.sref_threshold) {
                    auto addr = Address();
                    addr.rank = i;
                    auto cmd = Command(CommandType::SREF_ENTER, addr, -1);
                    if (channel_state_.IsReady(cmd, clk_)){
                        IssueCommand(cmd);
                        break;
                    }
                }
            }
        }
    }


    ScheduleTransaction();
    clk_++;
    cmd_queue_.ClockTick();
    return;
}

bool Controller::WillAcceptTransaction(uint64_t hex_addr, bool is_write) const {
    if (!is_write) {
        return read_queue_.size() < read_queue_.capacity();
    } else {
        return write_queue_.size() < write_queue_.capacity();
    }
}

bool Controller::AddTransaction(Transaction trans) {
    trans.added_cycle = clk_;
    // check if already in write buffer, can return immediately
    if (in_write_queue_.count(trans.addr) > 0) {
        if (trans.is_write) {
            stats_.num_writes_done++;
            write_callback_(trans.addr);
        } else {
            stats_.num_reads_done++;
            read_callback_(trans.addr);
        }
        return true;
    } else {
        if (trans.is_write) {
            // pretend it's done
            stats_.num_writes_done++;
            write_callback_(trans.addr);
            write_queue_.push_back(trans);
            in_write_queue_.insert(trans.addr);
            return write_queue_.size() <= write_queue_.capacity();
        } else {
            read_queue_.push_back(trans);
            return read_queue_.size() <= read_queue_.capacity();
        }
    }
}

void Controller::ScheduleTransaction() {
    // determine whether to schedule read or write
    if (write_draining_ == 0) {
        if (write_queue_.size() >= write_queue_.capacity() ||
            read_queue_.size() == 0) {
            write_draining_ = write_queue_.size();
        }
    }

    std::vector<Transaction>::iterator trans_it;
    std::vector<Transaction>::iterator queue_end;
    if (write_draining_ > 0) {
        trans_it = write_queue_.begin();
        queue_end = write_queue_.end();
    } else {
        trans_it = read_queue_.begin();
        queue_end = read_queue_.end();
    }

    while (trans_it != queue_end) {
        auto cmd = TransToCommand(*trans_it);
        if (cmd_queue_.WillAcceptCommand(cmd.Rank(), cmd.Bankgroup(),
                                         cmd.Bank())) {
            cmd_queue_.AddCommand(cmd);
            pending_queue_.insert(std::make_pair(cmd.id, *trans_it));
            cmd_id_++;
            if (cmd_id_ == std::numeric_limits<int>::max()) {
                cmd_id_ = 0;
            }
            if (cmd.IsRead()) {
                trans_it = read_queue_.erase(trans_it);
            } else {
                write_draining_ -= 1;
                trans_it = write_queue_.erase(trans_it);
            }
            break;  // only allow one transaction scheduled per cycle
        } else {
            ++trans_it;
        }
    }
}

void Controller::IssueCommand(const Command &tmp_cmd) {
    Command cmd = Command(tmp_cmd.cmd_type, tmp_cmd.addr, tmp_cmd.id);
#ifdef DEBUG_OUTPUT
    std::cout << std::left << std::setw(8) << clk_ << " " << cmd << std::endl;
#endif  // DEBUG_OUTPUT
#ifdef GENERATE_TRACE
    cmd_trace_ << std::left << std::setw(18) << clk_ << " " << cmd << std::endl;
#endif  // GENERATE_TRACE
#ifdef THERMAL
    // add channel in, only needed by thermal module
    cmd.addr.channel = channel_id_;
    thermal_calc_.UpdatePower(cmd, clk_);
#endif  // THERMAL
    // if read/write, update pending queue and return queue
    if (cmd.IsReadWrite()) {
        ProcessRWCommand(cmd);
    }
    channel_state_.UpdateTimingAndStates(cmd, clk_);
}

void Controller::ProcessRWCommand(const Command &cmd) {
    cmd_queue_.IssueRWCommand(cmd);
    auto it = pending_queue_.find(cmd.id);
    if (it == pending_queue_.end()) {
        std::cerr << cmd.id << " not in pending queue!" << std::endl;
        exit(1);
    }
    if (cmd.IsRead()) {
        it->second.complete_cycle =
            clk_ + config_.read_delay + config_.delay_queue_cycles;
        return_queue_.push_back(it->second);
        pending_queue_.erase(it);
    } else {
        // it->second.complete_cycle = clk_ + config_.write_delay +
        //                             config_.delay_queue_cycles;
        in_write_queue_.erase(it->second.addr);
        pending_queue_.erase(it);
    }
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
