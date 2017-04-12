#include "memory_system.h"

using namespace std;
using namespace dramcore;

MemorySystem::MemorySystem(std::function<void(uint64_t)> callback) :
    callback_(callback)
{
    ptr_config_ = new Config();
    auto config = *ptr_config_;
    ptr_timing_ = new Timing(config);
    ptr_stats_ = new Statistics();
    ctrls_.resize(config.channels);
    for(auto i = 0; i < config.channels; i++) {
        ctrls_[i] = new Controller(i, config, *ptr_timing_, *ptr_stats_, callback_);
    }
}

bool MemorySystem::InsertReq(uint64_t req_id, uint64_t hex_addr_, CommandType cmd_type) {
    auto addr = dramcore::AddressMapping(hex_addr_, *ptr_config_);
    Request* req = new Request(cmd_type, addr, -1, req_id); //TODO - Flip the order to maintain consistency. Figure out how to extract time from SST
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