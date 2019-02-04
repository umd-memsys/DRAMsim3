#include "dram_system.h"

#include <assert.h>

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
      timing_(config),
      num_channels_(config.channels),
      output_prefix_(config.output_prefix),
      output_level_(config.output_level),
      epoch_period_(config.epoch_period),
      shift_bits_(config.shift_bits),
      ch_width_(config.ch_width),
      ch_pos_(config.ch_pos),
#ifdef THERMAL
      thermal_calc_(config_),
#endif  // THERMAL
      clk_(0) {
    total_channels_ += num_channels_;

    std::string stats_txt_name = output_prefix_ + ".txt";
    std::string stats_csv_name = output_prefix_ + ".csv";
    std::string epoch_csv_name = output_prefix_ + "epoch.csv";
    std::string histo_csv_name = output_prefix_ + "hist.csv";

    if (output_level_ >= 0) {
        stats_txt_file_.open(stats_txt_name);
        stats_csv_file_.open(stats_csv_name);
    }

    if (output_level_ >= 1) {
        epoch_csv_file_.open(epoch_csv_name);
    }

    if (output_level_ >= 2) {
        histo_csv_file_.open(histo_csv_name);
    }

#ifdef GENERATE_TRACE
    std::string addr_trace_name("dramsim3addr.trace");
    address_trace_.open(addr_trace_name);
#endif
}

BaseDRAMSystem::~BaseDRAMSystem() {
    stats_txt_file_.close();
    stats_csv_file_.close();
    epoch_csv_file_.close();
#ifdef GENERATE_TRACE
    address_trace_.close();
#endif
}

int BaseDRAMSystem::GetChannel(uint64_t hex_addr) const {
    hex_addr >>= shift_bits_;
    return ModuloWidth(hex_addr, ch_width_, ch_pos_);
}

void BaseDRAMSystem::RegisterCallbacks(
    std::function<void(uint64_t)> read_callback,
    std::function<void(uint64_t)> write_callback) {
    // TODO this should be propagated to controllers
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

    ctrls_.reserve(num_channels_);
    for (auto i = 0; i < num_channels_; i++) {
#ifdef THERMAL
        ctrls_.push_back(new Controller(i, config_, timing_, thermal_calc_,
                                        read_callback_, write_callback_));
#else
        ctrls_.push_back(new Controller(i, config_, timing_, read_callback_,
                                        write_callback_));
#endif  // THERMAL
    }
}

JedecDRAMSystem::~JedecDRAMSystem() {
    for (auto it = ctrls_.begin(); it != ctrls_.end(); it++) {
        delete (*it);
    }
}

bool JedecDRAMSystem::WillAcceptTransaction(uint64_t hex_addr,
                                            bool is_write) const {
    int channel = GetChannel(hex_addr);
    return ctrls_[channel]->WillAcceptTransaction(hex_addr, is_write);
}

bool JedecDRAMSystem::AddTransaction(uint64_t hex_addr, bool is_write) {
// Record trace - Record address trace for debugging or other purposes
#ifdef ADDRESS_TRACE
    address_trace_ << left << setw(18) << clk_ << " " << setw(6) << std::hex
                   << (is_write ? "WRITE " : "READ ") << hex_addr << std::dec
                   << std::endl;
#endif

    int channel = GetChannel(hex_addr);
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

    if (clk_ % epoch_period_ == 0) {
        for (auto &&ctrl : ctrls_) {
            ctrl->PrintEpochStats(epoch_csv_file_);
        }
#ifdef THERMAL
        thermal_calc_.PrintTransPT(clk_);
#endif  // THERMAL
    }
    return;
}

void JedecDRAMSystem::PrintStats() {
    for (auto &&ctrl : ctrls_) {
        ctrl->PrintFinalStats(stats_txt_file_, stats_csv_file_, histo_csv_file_);
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
