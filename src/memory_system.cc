#include "memory_system.h"

using namespace std;
using namespace dramcore;

MemorySystem::MemorySystem(const string &config_file, std::function<void(uint64_t)> callback) :
    callback_(callback)
{
    ptr_config_ = new Config(config_file);
    //auto config = *ptr_config_;
    ptr_timing_ = new Timing(*ptr_config_);
    ptr_stats_ = new Statistics();
    ctrls_.resize((*ptr_config_).channels);
    for(auto i = 0; i < (*ptr_config_).channels; i++) {
        ctrls_[i] = new Controller(i, *ptr_config_, *ptr_timing_, *ptr_stats_, callback_);
    }
}

bool MemorySystem::InsertReq(uint64_t req_id, uint64_t hex_addr, bool is_write) {
    CommandType cmd_type = is_write ? CommandType::WRITE : CommandType ::READ;
    Request* req = new Request(cmd_type, hex_addr, *ptr_config_);
    return ctrls_[req->Channel()]->InsertReq(req);
}

void MemorySystem::ClockTick() {
    for( auto ctrl : ctrls_)
        ctrl->ClockTick();
    return;
}

void MemorySystem::PrintStats() {
    cout << "-----------------------------------------------------" << endl;
    cout << "Printing Statistics -- " << endl;
    cout << "-----------------------------------------------------" << endl;
    cout << *ptr_stats_;
    return;
}