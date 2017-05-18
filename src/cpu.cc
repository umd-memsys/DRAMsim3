#include "cpu.h"

using namespace std;
using namespace dramcore;

RandomCPU::RandomCPU(MemorySystem& memory_system) :
    memory_system_(memory_system),
    config_(*(memory_system.ptr_config_)),
    clk_(0)
{}


void RandomCPU::ClockTick()
{
    // Create random CPU requests at random time intervals
    // With random row buffer hits
    // And insert them into the controller
    
    // if (rand() % 4 != 0) {  // issue a request every 4 cycles
    //     clk_++;
    //     return;
    // }

    int hex_addr;
    bool is_write;
    bool issue_row_hit = (rand() % 5 == 0);  // every 5 reqs get a row hit

    if (issue_row_hit || (!get_next_)) { 
        hex_addr = last_addr_;  // use same address as last request to guarantee row hit
    } else {
        hex_addr = rand();
    }

    is_write = rand() % 3 == 0 ? true: false;  // R/W ratio 2:1

    bool is_inserted = memory_system_.InsertReq(req_id_, hex_addr, is_write);

    if (is_inserted) {
        req_id_++;
        get_next_ = true;
    } else {
        last_addr_ = hex_addr;  // failed to insert, keep the address
        get_next_ = false;
    }

    clk_++;
    return;
}

TraceBasedCPU::TraceBasedCPU(MemorySystem& memory_system, std::string trace_file) :
    memory_system_(memory_system),
    config_(*(memory_system.ptr_config_)),
    clk_(0)
{
    trace_file_.open(trace_file);
    if(trace_file_.fail()) {
        cerr << "Trace file does not exist" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
}

void TraceBasedCPU::ClockTick() {
    clk_++;
    if(!trace_file_.eof()) {
        if(get_next_) {
            get_next_ = false;
            Access access;
            trace_file_ >> access;
            req_ = FormRequest(access);
        }
        if(req_->arrival_time_ <= clk_) {
            if(memory_system_.ctrls_[req_->Channel()]->InsertReq(req_)) { //TODO - This is silly. Modify it.
                get_next_ = true;
                req_id_++;
            }
        }
    }
    return;
}

Request* TraceBasedCPU::FormRequest(const Access& access) {
    auto addr = AddressMapping(access.hex_addr_, config_);
    CommandType cmd_type;
    if( access.access_type_ == "READ")
        cmd_type = CommandType::READ;
    else if( access.access_type_ == "WRITE")
        cmd_type = CommandType::WRITE;
    else {
        cerr << "Unknown access type, should be either READ or WRITE" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
    return new Request(cmd_type, addr, access.time_, req_id_);
}

