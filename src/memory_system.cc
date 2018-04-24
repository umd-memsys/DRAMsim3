#include "memory_system.h"

namespace dramsim3 {
MemorySystem::MemorySystem(const std::string &config_file,
                           const std::string &output_dir,
                           std::function<void(uint64_t)> read_callback,
                           std::function<void(uint64_t)> write_callback) {
    config_ = new Config(config_file, output_dir);
    // TODO: ideal memory type?
    if (config_->IsHMC()) {
        dram_system_ = new HMCMemorySystem(config_file, output_dir,
                                           read_callback, write_callback);
    } else {
        dram_system_ = new JedecDRAMSystem(config_file, output_dir,
                                           read_callback, write_callback);
    }

}

MemorySystem::~MemorySystem() { 
    delete (config_);
    delete (dram_system_);
}

void MemorySystem::ClockTick() {
    dram_system_->ClockTick();
}

BaseDRAMSystem *MemorySystem::GetDRAMSystem() { return dram_system_; }

double MemorySystem::GetTCK() const {
    return config_->tCK;
}

int MemorySystem::GetBusBits() const {
    return config_->bus_width;
}

int MemorySystem::GetBurstLength() const {
    return config_->BL;
}

int MemorySystem::GetQueueSize() const {
    return config_->queue_size;
}


void MemorySystem::RegisterCallbacks(
        std::function<void(uint64_t)> read_callback,
        std::function<void(uint64_t)> write_callback) {
    dram_system_->RegisterCallbacks(read_callback, write_callback);
}

bool MemorySystem::IsInsertable() const {
    return true;
}

bool MemorySystem::InsertRequest(bool is_write, uint64_t addr) {
    return dram_system_->InsertReq(addr, is_write);
}

void MemorySystem::PrintStats() const {
    dram_system_->PrintStats();
}

}  // namespace dramsim3

// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C" {
    void libdramsim3_is_present(void) { ; }
}

