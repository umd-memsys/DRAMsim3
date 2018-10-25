#include "dram_system.h"

#include <assert.h>

#ifdef GENERATE_TRACE
#include "../ext/fmt/src/format.h"
#endif  // GENERATE_TRACE

namespace dramsim3 {

// alternative way is to assign the id in constructor but this is less
// destructive
int BaseDRAMSystem::num_mems_ = 0;

BaseDRAMSystem::BaseDRAMSystem(Config &config, const std::string &output_dir,
                               std::function<void(uint64_t)> read_callback,
                               std::function<void(uint64_t)> write_callback)
    : read_callback_(read_callback),
      write_callback_(write_callback),
      clk_(0),
      last_req_clk_(0),
      config_(config),
      timing_(config_),
#ifdef THERMAL
      stats_(config_),
      thermal_calc_(config_, stats_) {
#else
      stats_(config_) {
#endif  // THERMAL
    mem_sys_id_ = num_mems_;
    num_mems_ += 1;

    if (mem_sys_id_ > 0) {
        // if there are more than one memory_systems then rename the output to
        // preven being overwritten
        RenameFileWithNumber(config_.stats_file, mem_sys_id_);
        RenameFileWithNumber(config_.epoch_stats_file, mem_sys_id_);
        RenameFileWithNumber(config_.stats_file_csv, mem_sys_id_);
        RenameFileWithNumber(config_.epoch_stats_file_csv, mem_sys_id_);
    }

    if (config_.output_level >= 0) {
        stats_file_.open(config_.stats_file);
        stats_file_csv_.open(config_.stats_file_csv);
    }

    if (config_.output_level >= 1) {
        epoch_stats_file_csv_.open(config_.epoch_stats_file_csv);
        stats_.PrintStatsCSVHeader(epoch_stats_file_csv_);
    }

    if (config_.output_level >= 2) {
        histo_stats_file_csv_.open(config_.histo_stats_file_csv);
    }

    if (config_.output_level >= 3) {
        epoch_stats_file_.open(config_.epoch_stats_file);
    }

#ifdef GENERATE_TRACE
    std::string addr_trace_name(config_.output_prefix + "addr.trace");
    address_trace_.open(addr_trace_name);
#endif
}

BaseDRAMSystem::~BaseDRAMSystem() {
    stats_file_.close();
    epoch_stats_file_.close();
    stats_file_csv_.close();
    epoch_stats_file_csv_.close();

#ifdef GENERATE_TRACE
    address_trace_.close();
#endif
}

void BaseDRAMSystem::RegisterCallbacks(
    std::function<void(uint64_t)> read_callback,
    std::function<void(uint64_t)> write_callback) {
    read_callback_ = read_callback;
    write_callback_ = write_callback;
}

void BaseDRAMSystem::PrintIntermediateStats() {
    if (config_.output_level >= 1) {
        stats_.PrintEpochStatsCSVFormat(epoch_stats_file_csv_);
    }

    if (config_.output_level >= 2) {
        stats_.PrintEpochHistoStatsCSVFormat(histo_stats_file_csv_);
    }

    if (config_.output_level >= 3) {
        epoch_stats_file_
            << "-----------------------------------------------------"
            << std::endl;
        epoch_stats_file_ << "Epoch stats from clock = "
                          << clk_ - config_.epoch_period << " to " << clk_
                          << std::endl;
        epoch_stats_file_
            << "-----------------------------------------------------"
            << std::endl;
        stats_.PrintEpochStats(epoch_stats_file_);
        epoch_stats_file_
            << "-----------------------------------------------------"
            << std::endl;
    }
    return;
}

void BaseDRAMSystem::PrintStats() {
    // update one last time before print
    stats_.PreEpochCompute(clk_);
    stats_.UpdateEpoch(clk_);
    std::cout << "-----------------------------------------------------"
              << std::endl;
    std::cout << "Printing final stats of JedecDRAMSystem " << mem_sys_id_
              << " -- " << std::endl;
    std::cout << "-----------------------------------------------------"
              << std::endl;
    std::cout << stats_;
    std::cout << "-----------------------------------------------------"
              << std::endl;
    if (config_.output_level >= 0) {
        stats_.PrintStats(stats_file_);
        // had to print the header here instead of the constructor
        // because histogram headers are only known at the end
        stats_.PrintStatsCSVHeader(stats_file_csv_);
        stats_.PrintStatsCSVFormat(stats_file_csv_);
#ifdef THERMAL
        thermal_calc_.PrintFinalPT(clk_);
#endif  // THERMAL
    }
    return;
}

JedecDRAMSystem::JedecDRAMSystem(Config &config, const std::string &output_dir,
                                 std::function<void(uint64_t)> read_callback,
                                 std::function<void(uint64_t)> write_callback)
    : BaseDRAMSystem(config, output_dir, read_callback, write_callback) {
    if (config_.IsHMC()) {
        std::cerr << "Initialized a memory system with an HMC config file!"
                  << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    ctrls_.reserve(config_.channels);
    for (auto i = 0; i < config_.channels; i++) {
#ifdef THERMAL
        ctrls_.emplace_back(i, config_, timing_, stats_, thermal_calc_,
                            read_callback_, write_callback_);
#else
        ctrls_.emplace_back(i, config_, timing_, stats_, read_callback_,
                            write_callback_);
#endif  // THERMAL
    }
}

JedecDRAMSystem::~JedecDRAMSystem() {}

bool JedecDRAMSystem::WillAcceptTransaction(uint64_t hex_addr,
                                            bool is_write) const {
    int channel = MapChannel(hex_addr);
    return ctrls_[channel].WillAcceptTransaction(hex_addr, is_write);
}

bool JedecDRAMSystem::AddTransaction(uint64_t hex_addr, bool is_write) {
// Record trace - Record address trace for debugging or other purposes
#ifdef GENERATE_TRACE
    address_trace_ << left << setw(18) << clk_ << " " << setw(6) << std::hex
                   << (is_write ? "WRITE " : "READ ") << hex_addr << std::dec
                   << std::endl;
#endif

    int channel = MapChannel(hex_addr);
    bool ok = ctrls_[channel].WillAcceptTransaction(hex_addr, is_write);

#ifdef NO_BACKPRESSURE
    // Some CPU simulators might not model the backpressure because queues are
    // full. To make them work we push them to the transaction queue anyway
    ok = true;
    stats_.num_buffered_trans++;
#endif
    assert(ok);
    if (ok) {
        Transaction trans = Transaction(hex_addr, is_write);
        ctrls_[channel].AddTransaction(trans);
        stats_.interarrival_latency.AddValue(clk_ - last_req_clk_);
        last_req_clk_ = clk_;
    }
    return ok;
}

void JedecDRAMSystem::ClockTick() {
    if (clk_ % config_.epoch_period == 0 && clk_ != 0) {
        // calculate queue usage each epoch
        // otherwise it would be too inefficient
        int queue_usage_total = 0;
        for (const auto &ctrl : ctrls_) {
            queue_usage_total += ctrl.QueueUsage();
        }
        stats_.queue_usage.epoch_value = static_cast<double>(queue_usage_total);
        stats_.PreEpochCompute(clk_);
        PrintIntermediateStats();
        stats_.UpdateEpoch(clk_);
    }

    for (auto &&ctrl : ctrls_) ctrl.ClockTick();

    clk_++;
    stats_.dramcycles++;
    return;
}

IdealDRAMSystem::IdealDRAMSystem(Config &config, const std::string &output_dir,
                                 std::function<void(uint64_t)> read_callback,
                                 std::function<void(uint64_t)> write_callback)
    : BaseDRAMSystem(config, output_dir, read_callback, write_callback),
      latency_(config_.ideal_memory_latency) {}

IdealDRAMSystem::~IdealDRAMSystem() {}

bool IdealDRAMSystem::AddTransaction(uint64_t hex_addr, bool is_write) {
    auto trans = Transaction(hex_addr, is_write);
    trans.added_cycle = clk_;
    infinite_buffer_q_.push_back(trans);
    return true;
}

void IdealDRAMSystem::ClockTick() {
    stats_.dramcycles++;
    for (auto trans_it = infinite_buffer_q_.begin();
         trans_it != infinite_buffer_q_.end();) {
        if (clk_ - trans_it->added_cycle >= static_cast<uint64_t>(latency_)) {
            if (trans_it->is_write) {
                stats_.num_writes_done++;
                write_callback_(trans_it->addr);
            } else {
                stats_.num_reads_done++;
                read_callback_(trans_it->addr);
            }
            stats_.access_latency.AddValue(clk_ - trans_it->added_cycle);
            trans_it = infinite_buffer_q_.erase(trans_it++);
        }
        if (trans_it != infinite_buffer_q_.end()) {
            ++trans_it;
        }
    }

    if (clk_ % config_.epoch_period == 0) {
        PrintIntermediateStats();
        stats_.UpdateEpoch(clk_);
    }
    clk_++;
    return;
}

}  // namespace dramsim3
