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
        last_addr_ = gen();
    }
    bool is_write = (gen() % 3 == 0);  // R/W ratio 2:1
    bool get_next_ = memory_system_.IsReqInsertable(last_addr_, is_write);
    if(get_next_) {
        memory_system_.InsertReq(last_addr_, is_write);
    }
    return;
}

void StreamCPU::ClockTick() { 
    // stream-add, read 2 arrays, add them up to the third array
    // this is a very simple approximate but should be able to produce 
    // enough buffer hits

    // moving on to next set of arrays
    if (offset_ >= array_size_) {
        addr_a_ = gen();
        addr_b_ = gen();
        addr_c_ = gen();
        offset_ = 0;
    }

    if (!inserted_a_ && memory_system_.IsReqInsertable(addr_a_ + offset_, false)){
        memory_system_.InsertReq(addr_a_ + offset_, false);
        inserted_a_ = true;
    } else {
        inserted_a_ = false;
    }
    if (!inserted_b_ && memory_system_.IsReqInsertable(addr_b_ + offset_, false)){
        memory_system_.InsertReq(addr_b_ + offset_, false);
        inserted_b_ = true;
    } else {
        inserted_b_ = false;
    }
    if (!inserted_c_ && memory_system_.IsReqInsertable(addr_c_ + offset_, true)){
        memory_system_.InsertReq(addr_c_ + offset_, true);
        inserted_c_ = true;
    } else {
        inserted_c_ = false;
    }

    // moving on to next element
    if (inserted_a_ && inserted_b_ && inserted_c_) {
        offset_ += stride_;
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
            bool is_write = access_.access_type_ == "WRITE";
            bool get_next_ = memory_system_.IsReqInsertable(access_.hex_addr_, is_write);
            if(get_next_) {
                memory_system_.InsertReq(access_.hex_addr_, is_write);
            }
        }
    }
    return;
}

ThermalCPU::ThermalCPU(BaseMemorySystem& memory_system):
    CPU(memory_system),
    curr_index_(0),
    sent_successful_(true)
{
    //                    |----row-----|ba|---col|--|
    uint64_t base_addr = 0b01000000000000101000001000000000;
    uint64_t one_row  =  0b01000000000010000000000000000000;
    uint64_t one_bank  = 0b00000000000000010000000000000000; 
    uint64_t one_col  =  0b00000000000000000000001000000000;
    int pattern[] = {
        base_addr,
        base_addr + one_col,
        base_addr + one_col * 2,
        base_addr + one_col * 3,
        base_addr + one_bank,
        base_addr - one_bank,
    };
    pattern_len_ = sizeof(pattern) / sizeof(pattern[0]);
    for (int i = 0; i < pattern_len_; i++) {
        addr_pattern_.push_back(pattern[i]);
    }
}

void ThermalCPU::ClockTick() {
    // keep repeating an access pattern and hope to break something
    auto next_addr = addr_pattern_[curr_index_];
    sent_successful_ = memory_system_.IsReqInsertable(next_addr, false);
    if (sent_successful_) {
        memory_system_.InsertReq(next_addr, false);
        curr_index_ ++;
        if (curr_index_ == pattern_len_) {
            curr_index_ = 0;
        }
    }
    clk_ ++;
}

