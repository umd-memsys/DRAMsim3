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

StreamCPU::StreamCPU(BaseMemorySystem& memory_system) :
    CPU(memory_system)
{}

void RandomCPU::ClockTick() {
    // Create random CPU requests at full speed
    // this is useful to exploit the parallelism of a DRAM protocol
    // and is also immune to address mapping and scheduling policies
    clk_++;
    if (get_next_) {
        last_addr_ = rand();
    }
    bool is_write = (rand() % 3 == 0);  // R/W ratio 2:1
    get_next_ = memory_system_.InsertReq(last_addr_, is_write);
    return;
}

void StreamCPU::ClockTick() {
    // stream-add, read 2 arrays, add them up to the third array
    // this is a very simple approximate but should be able to produce 
    // enough buffer hits
    if (next_location_) {
        // jump to a new location and start
        next_location_ = false;
        addr_a_ = rand();
        addr_b_ = rand();
        addr_c_ = rand();
        offset_ = 0;
    }
    if (next_element_) {
        bool inserted_a = memory_system_.InsertReq(addr_a_ + offset_, false);
        bool inserted_b = memory_system_.InsertReq(addr_b_ + offset_, false);
        bool inserted_c = memory_system_.InsertReq(addr_c_ + offset_, true);
        offset_ += 8;
        all_inserted_ = inserted_a && inserted_b && inserted_c;
    } else {
        if (all_inserted_) {
            next_element_ = (rand() % 50 == 0); 
        } else {
            next_element_ = (rand() % 20 == 0);
        }
    }
    if (offset_ >= array_size_) {
        next_location_ = true;
    }
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
            if(memory_system_.InsertReq(access_.hex_addr_, access_.access_type_ == "WRITE")) {
                get_next_ = true;
            }
        }
    }
    return;
}
