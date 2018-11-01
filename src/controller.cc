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
                       std::function<void(uint64_t)> read_callback,
                       std::function<void(uint64_t)> write_callback)
#endif  // THERMAL
    : read_callback_(read_callback),
      write_callback_(write_callback),
      channel_id_(channel),
      clk_(0),
      config_(config),
      stats_(config_, channel),
      channel_state_(config, timing, stats_),
      cmd_queue_(channel_id_, config, channel_state_, stats_),
      refresh_(config, channel_state_),
      is_unified_queue_(config.unified_queue),
#ifdef THERMAL
      thermal_calc_(thermal_calc),
#endif  // THERMAL
      row_buf_policy_(config.row_buf_policy == "CLOSE_PAGE"
                          ? RowBufPolicy::CLOSE_PAGE
                          : RowBufPolicy::OPEN_PAGE),
      last_trans_clk_(0),
      write_draining_(0) {
    if (is_unified_queue_) {
        unified_queue_.reserve(config_.trans_queue_size);
    } else {
        read_queue_.reserve(config_.trans_queue_size);
        write_queue_.reserve(config_.trans_queue_size);
    }

    // this prefix includes the output directory
    std::string channel_str = "_" + std::to_string(channel_id_);
    std::string pre = config_.output_prefix + channel_str;
    std::string stats_txt_name = pre + ".txt";
    std::string stats_csv_name = pre + ".csv";
    std::string epoch_txt_name = pre + "epoch.txt";
    std::string epoch_csv_name = pre + "epoch.csv";
    std::string histo_csv_name = pre + "hist.csv";

    if (config_.output_level >= 0) {
        stats_txt_file_.open(stats_txt_name);
        stats_csv_file_.open(stats_csv_name);
    }

    if (config_.output_level >= 1) {
        epoch_csv_file_.open(epoch_csv_name);
        stats_.PrintStatsCSVHeader(epoch_csv_file_);
    }

    if (config_.output_level >= 2) {
        histo_csv_file_.open(histo_csv_name);
    }

    // if (config_.output_level >= 3) {
    //     epoch_txt_file_.open(config_.epoch_stats_file);
    // }

#ifdef GENERATE_TRACE
    std::string trace_file_name = "dramsim3ch_" + channel_str + "cmd.trace";
    std::cout << "Command Trace write to " << trace_file_name << std::endl;
    cmd_trace_.open(trace_file_name, std::ofstream::out);
#endif  // GENERATE_TRACE
}

Controller::~Controller() {
    stats_txt_file_.close();
    epoch_txt_file_.close();
    stats_csv_file_.close();
    epoch_csv_file_.close();
#ifdef GENERATE_TRACE
    cmd_trace_.close();
#endif
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
            stats_.sref_cycles[i]++;
        } else {
            bool all_idle = channel_state_.IsAllBankIdleInRank(i);
            if (all_idle) {
                stats_.all_bank_idle_cycles[i]++;
                channel_state_.rank_idle_cycles[i] += 1;
            } else {
                stats_.active_cycles[i]++;
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
                    if (channel_state_.IsReady(cmd, clk_)) {
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
                    if (channel_state_.IsReady(cmd, clk_)) {
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
    stats_.dramcycles++;
    if (clk_ % config_.epoch_period == 0) {
        PrintEpochStats();
    }
    return;
}

bool Controller::WillAcceptTransaction(uint64_t hex_addr, bool is_write) const {
    if (is_unified_queue_) {
        return unified_queue_.size() < unified_queue_.capacity();
    } else if (!is_write) {
        return read_queue_.size() < read_queue_.capacity();
    } else {
        return write_queue_.size() < write_queue_.capacity();
    }
}

bool Controller::AddTransaction(Transaction trans) {
    trans.added_cycle = clk_;
    stats_.interarrival_latency.AddValue(clk_ - last_trans_clk_);
    last_trans_clk_ = clk_;

    // check if already in write buffer, can return immediately
    if (in_write_buf_.count(trans.addr) > 0) {
        stats_.num_write_buf_hits++;
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
            in_write_buf_.insert(trans.addr);
            if (is_unified_queue_) {
                unified_queue_.push_back(trans);
            } else {
                write_queue_.push_back(trans);
            }
        } else {
            if (is_unified_queue_) {
                unified_queue_.push_back(trans);
            } else {
                read_queue_.push_back(trans);
            }
        }
        return true;
    }
}

void Controller::ScheduleTransaction() {
    // determine whether to schedule read or write
    if (write_draining_ == 0 && !is_unified_queue_) {
        // TODO need better switching machnism
        if (write_queue_.size() >= write_queue_.capacity() ||
            (read_queue_.size() == 0 && write_queue_.size() >= 8)) {
            write_draining_ = write_queue_.size();
        }
    }

    std::vector<Transaction>::iterator trans_it;
    std::vector<Transaction>::iterator queue_end;
    if (is_unified_queue_) {
        trans_it = unified_queue_.begin();
        queue_end = unified_queue_.end();
    } else if (write_draining_ > 0) {
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
            pending_queue_.insert(std::make_pair(trans_it->addr, *trans_it));
            if (is_unified_queue_) {
                unified_queue_.erase(trans_it);
            } else if (cmd.IsRead()) {
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
    Command cmd = Command(tmp_cmd.cmd_type, tmp_cmd.addr, tmp_cmd.hex_addr);
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
        auto it = pending_queue_.find(cmd.hex_addr);
        if (it == pending_queue_.end()) {
            std::cerr << cmd.hex_addr << " not in pending queue!" << std::endl;
            exit(1);
        }
        if (cmd.IsRead()) {
            it->second.complete_cycle =
                clk_ + config_.read_delay + config_.delay_queue_cycles;
            return_queue_.push_back(it->second);
        } else {
            // it->second.complete_cycle = clk_ + config_.write_delay +
            //                             config_.delay_queue_cycles;
            in_write_buf_.erase(it->second.addr);
        }
        pending_queue_.erase(it);
    }
    channel_state_.UpdateTimingAndStates(cmd, clk_);
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
    Command cmd = Command(cmd_type, addr, trans.addr);
    return cmd;
}

int Controller::QueueUsage() const { return cmd_queue_.QueueUsage(); }

void Controller::PrintEpochStats() {
    stats_.PreEpochCompute(clk_);
    if (config_.output_level >= 1) {
        stats_.PrintEpochStatsCSVFormat(epoch_csv_file_);
    }

    if (config_.output_level >= 2) {
        stats_.PrintEpochHistoStatsCSVFormat(histo_csv_file_);
    }

    stats_.UpdateEpoch(clk_);
    return;
}

void Controller::PrintFinalStats() {
    stats_.PreEpochCompute(clk_);
    stats_.UpdateEpoch(clk_);
    std::cout << "-----------------------------------------------------"
              << std::endl;
    std::cout << "Printing final stats of Channel " << channel_id_ << std::endl;
    std::cout << "-----------------------------------------------------"
              << std::endl;
    std::cout << stats_;
    std::cout << "-----------------------------------------------------"
              << std::endl;
    if (config_.output_level >= 0) {
        stats_.PrintStats(stats_txt_file_);
        stats_.PrintStatsCSVHeader(stats_csv_file_);
        stats_.PrintStatsCSVFormat(stats_csv_file_);
#ifdef THERMAL
        thermal_calc_.PrintFinalPT(clk_);
#endif  // THERMAL
    }
    return;
}

}  // namespace dramsim3
