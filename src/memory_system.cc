#include "memory_system.h"
#include <iostream>
#include <iomanip> 
#include <locale>

namespace dramsim3 {
MemorySystem::MemorySystem(const std::string &config_file,
                           const std::string &output_dir,
                           std::function<void(uint64_t)> read_callback,
                           std::function<void(uint64_t)> write_callback)
    : config_(new Config(config_file, output_dir)) {
    #ifdef MY_DEBUG
    std::cout<<"== "<<__func__<<" == ";
    std::cout<<"constructor"<<std::endl;
    #endif
    // TODO: ideal memory type?
    if (config_->IsHMC()) {
        dram_system_ = new HMCMemorySystem(*config_, output_dir, read_callback,
                                           write_callback);
    } else {
        dram_system_ = new JedecDRAMSystem(*config_, output_dir, read_callback,
                                           write_callback);
    }
}

MemorySystem::~MemorySystem() {
    delete (dram_system_);
    delete (config_);
}

void MemorySystem::ClockTick() { dram_system_->ClockTick(); }

double MemorySystem::GetTCK() const { return config_->tCK; }

int MemorySystem::GetBusBits() const { return config_->bus_width; }

int MemorySystem::GetBurstLength() const { return config_->BL; }

int MemorySystem::GetQueueSize() const { return config_->trans_queue_size; }

int MemorySystem::GetRank(uint64_t hex_addr) const {
    auto addr = config_->AddressMapping(hex_addr);
    return addr.rank;
}

int MemorySystem::GetNumChannel() const {
    std::cout<<__func__<<std::endl;
    std::cout<<"Channel :"<<config_->channels<<std::endl;
    return config_->channels;
}


void MemorySystem::RegisterCallbacks(
    std::function<void(uint64_t)> read_callback,
    std::function<void(uint64_t)> write_callback) {
    dram_system_->RegisterCallbacks(read_callback, write_callback);
}

bool MemorySystem::WillAcceptTransaction(uint64_t hex_addr,
                                         bool is_write) const {
    return dram_system_->WillAcceptTransaction(hex_addr, is_write);
}

bool MemorySystem::WillAcceptTransaction(uint64_t hex_addr,
                                         bool is_write, bool is_mrs) const {
    return dram_system_->WillAcceptTransaction(hex_addr, is_write, is_mrs);
}

bool MemorySystem::AddTransaction(uint64_t hex_addr, bool is_write) {
    return dram_system_->AddTransaction(hex_addr, is_write, false);
}

bool MemorySystem::AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS) {
    return dram_system_->AddTransaction(hex_addr, is_write, is_MRS);
}

bool MemorySystem::AddTransaction(uint64_t hex_addr, bool is_write, bool is_MRS, std::vector<u_int64_t> &payload) {
    return dram_system_->AddTransaction(hex_addr, is_write, is_MRS, payload);
}

void MemorySystem::PrintStats() const { dram_system_->PrintStats(); }

void MemorySystem::ResetStats() { dram_system_->ResetStats(); }

bool MemorySystem::isLRDIMM() {
    return config_->is_LRDIMM;
}

Config* MemorySystem::GetConfig() {
    return config_;
}


MemorySystem* GetMemorySystem(const std::string &config_file, const std::string &output_dir,
                 std::function<void(uint64_t)> read_callback,
                 std::function<void(uint64_t)> write_callback) {
    return new MemorySystem(config_file, output_dir, read_callback, write_callback);
}

std::vector<uint64_t> MemorySystem::GetRespData(uint64_t hex_addr) {
     return dram_system_->GetRespData(hex_addr); 
}

}  // namespace dramsim3

// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C" {
void libdramsim3_is_present(void) { ; }
}
