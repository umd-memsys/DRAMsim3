#include "cpu.h"

using namespace std;
using namespace dramcore;


CPU::CPU(BaseMemorySystem& memory_system) :
    memory_system_(memory_system),
    clk_(0)
{}

RandomCPU::RandomCPU(BaseMemorySystem& memory_system) :
    CPU(memory_system)
{}

void RandomCPU::ClockTick() {
    // Create random CPU requests at random time intervals
    // With random row buffer hits
    // And insert them into the controller

    clk_++;
    bool issue_row_hit = (rand() % 5 == 0);  // every 5 reqs get a row hit
    if(!issue_row_hit && get_next_) {
        last_addr_ = rand();
    }
    bool is_write = (rand() % 3 == 0);  // R/W ratio 2:1
    get_next_ = memory_system_.InsertReq(0, last_addr_, is_write);
    return;
}

TraceBasedCPU::TraceBasedCPU(BaseMemorySystem& memory_system, std::string trace_file) :
    CPU(memory_system)
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
            trace_file_ >> access_;
        }
        if(access_.time_ <= clk_) {
            if(memory_system_.InsertReq(0, access_.hex_addr_, access_.access_type_ == "WRITE")) {
                get_next_ = true;
            }
        }
    }
    return;
}
