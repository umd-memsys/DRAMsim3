#include "memory_system.h"

using namespace std;
using namespace dramcore;

MemorySystem::MemorySystem(const string &config_file, std::function<void(uint64_t)> callback) :
    callback_(callback)
{
    ptr_config_ = new Config(config_file);
    ptr_timing_ = new Timing(*ptr_config_);
    ptr_stats_ = new Statistics();
    ctrls_.resize(ptr_config_->channels);
    for(auto i = 0; i < ptr_config_->channels; i++) {
        ctrls_[i] = new Controller(i, *ptr_config_, *ptr_timing_, *ptr_stats_, callback_);
    }
}

MemorySystem::~MemorySystem() {
    for(auto i = 0; i < ptr_config_->channels; i++) {
        delete(ctrls_[i]);
    }
    delete(ptr_stats_);
    delete(ptr_timing_);
    delete(ptr_config_);
}

bool MemorySystem::InsertReq(uint64_t req_id, uint64_t hex_addr, bool is_write) {
    CommandType cmd_type = is_write ? CommandType::WRITE : CommandType ::READ;
    id_++;
    Request* req = new Request(cmd_type, hex_addr, *ptr_config_, clk_, id_);
    /*
    Address addr = AddressMapping(hex_addr, *ptr_config_);
    Request* req = new Request(cmd_type, addr);
     */

    // Some CPU simulators might not model the backpressure because queues are full.
    // An approximate way of addressing this scenario is to buffer all such requests here in the DRAM simulator and then
    // feed them into the actual memory controller queues as and when space becomes available.
    // Note - This is an approximation and if the size of such buffer queue becomes large during the course of the
    // simulation, then the accuracy sought of devolves into that of a trace based simulation.
    bool is_insertable = ctrls_[req->Channel()]->InsertReq(req);
    if((*ptr_config_).req_buffering_enabled && !is_insertable) {
        buffer_q_.push_back(req);
        is_insertable = true;
        numb_buffered_requests++;
    }
    return is_insertable;
}

void MemorySystem::ClockTick() {
    clk_++;
    for( auto ctrl : ctrls_)
        ctrl->ClockTick();

    //Insert requests stored in the buffer_q as and when space is available
    if(!buffer_q_.empty()) {
        for(auto req_itr = buffer_q_.begin(); req_itr != buffer_q_.end(); req_itr++) {
            auto req = *req_itr;
            if(ctrls_[req->Channel()]->InsertReq(req)) {
                buffer_q_.erase(req_itr);
                break;  // either break or set req_itr to the return value of erase() 
            }
        }
    }
    return;
}

void MemorySystem::PrintStats() {
    cout << "-----------------------------------------------------" << endl;
    cout << "Printing Statistics -- " << endl;
    cout << "-----------------------------------------------------" << endl;
    cout << *ptr_stats_;
    cout << "numb_buffered_requests=" << numb_buffered_requests << endl;
    return;
}


// This function can be used by autoconf AC_CHECK_LIB since
// apparently it can't detect C++ functions.
// Basically just an entry in the symbol table
extern "C"
{
void libdramcore_is_present(void)
{
    ;
}
}