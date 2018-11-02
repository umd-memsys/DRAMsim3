#include "dram_system.h"

#include <assert.h>

#ifdef GENERATE_TRACE
#include "../ext/fmt/src/format.h"
#endif  // GENERATE_TRACE

namespace dramsim3 {

// alternative way is to assign the id in constructor but this is less
// destructive
int BaseDRAMSystem::total_channels_ = 0;

BaseDRAMSystem::BaseDRAMSystem(Config &config, const std::string &output_dir,
                               std::function<void(uint64_t)> read_callback,
                               std::function<void(uint64_t)> write_callback)
    : read_callback_(read_callback),
      write_callback_(write_callback),
      last_req_clk_(0),
      config_(config),
      timing_(config_),
#ifdef THERMAL
      thermal_calc_(config_, stats_),
#endif  // THERMAL
      clk_(0) {
    total_channels_ += config_.channels;

    std::string stats_txt_name = config_.output_prefix + ".txt";
    std::string stats_csv_name = config_.output_prefix + ".csv";
    std::string epoch_txt_name = config_.output_prefix + "epoch.txt";
    std::string epoch_csv_name = config_.output_prefix + "epoch.csv";
    std::string histo_csv_name = config_.output_prefix + "hist.csv";

    if (config_.output_level >= 0) {
        stats_txt_file_.open(stats_txt_name);
        stats_csv_file_.open(stats_csv_name);
    }

    if (config_.output_level >= 1) {
        epoch_csv_file_.open(epoch_csv_name);
    }

    // TODO Not working
    // if (config_.output_level >= 2) {
    //     histo_csv_file_.open(histo_csv_name);
    // }

    // if (config_.output_level >= 3) {
    //     epoch_txt_file_.open(config_.epoch_stats_file);
    // }
#ifdef GENERATE_TRACE
    std::string addr_trace_name("dramsim3addr.trace");
    address_trace_.open(addr_trace_name);
#endif
}

BaseDRAMSystem::~BaseDRAMSystem() {
    stats_txt_file_.close();
    epoch_txt_file_.close();
    stats_csv_file_.close();
    epoch_csv_file_.close();
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
        ctrls_.push_back(new Controller(i, config_, timing_, thermal_calc_,
                                        read_callback_, write_callback_));
#else
        ctrls_.push_back(new Controller(i, config_, timing_, read_callback_,
                                        write_callback_));
#endif  // THERMAL
    }

    // Only print one header at the beginning
    if (config_.output_level >= 0) {
        ctrls_[0]->PrintCSVHeader(stats_csv_file_);
    }
    if (config_.output_level >= 1) {
        ctrls_[0]->PrintCSVHeader(epoch_csv_file_);
    }
}

JedecDRAMSystem::~JedecDRAMSystem() {
    // TODO deleting causes seg fault
    for (auto &&ctrl_ptr : ctrls_) {
        delete(ctrl_ptr);
    }
}

bool JedecDRAMSystem::WillAcceptTransaction(uint64_t hex_addr,
                                            bool is_write) const {
    int channel = MapChannel(hex_addr);
    return ctrls_[channel]->WillAcceptTransaction(hex_addr, is_write);
}

bool JedecDRAMSystem::AddTransaction(uint64_t hex_addr, bool is_write) {
// Record trace - Record address trace for debugging or other purposes
#ifdef GENERATE_TRACE
    address_trace_ << left << setw(18) << clk_ << " " << setw(6) << std::hex
                   << (is_write ? "WRITE " : "READ ") << hex_addr << std::dec
                   << std::endl;
#endif

    int channel = MapChannel(hex_addr);
    bool ok = ctrls_[channel]->WillAcceptTransaction(hex_addr, is_write);

    assert(ok);
    if (ok) {
        Transaction trans = Transaction(hex_addr, is_write);
        ctrls_[channel]->AddTransaction(trans);
    }
    return ok;
}

void JedecDRAMSystem::ClockTick() {
    for (auto &&ctrl : ctrls_) ctrl->ClockTick();

    clk_++;

    if (clk_ % config_.epoch_period == 0) {
        for (auto &&ctrl : ctrls_) {
            ctrl->PrintEpochStats(epoch_csv_file_, histo_csv_file_);
        }
    }
    return;
}

void JedecDRAMSystem::PrintStats() {
    for (auto &&ctrl : ctrls_) {
        ctrl->PrintFinalStats(stats_txt_file_, stats_csv_file_);
    }
#ifdef THERMAL
    thermal_calc_.PrintFinalPT(clk_);
#endif  // THERMAL
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
    for (auto trans_it = infinite_buffer_q_.begin();
         trans_it != infinite_buffer_q_.end();) {
        if (clk_ - trans_it->added_cycle >= static_cast<uint64_t>(latency_)) {
            if (trans_it->is_write) {
                write_callback_(trans_it->addr);
            } else {
                read_callback_(trans_it->addr);
            }
            trans_it = infinite_buffer_q_.erase(trans_it++);
        }
        if (trans_it != infinite_buffer_q_.end()) {
            ++trans_it;
        }
    }

    clk_++;
    return;
}

}  // namespace dramsim3
