#ifndef __CPU_H
#define __CPU_H

#include <fstream>
#include <vector>
#include "controller.h"
#include "common.h"
#include "configuration.h"
#include "memory_system.h"

namespace dramcore {

class CPU {
public:
    CPU(MemorySystem& memory_system);
    virtual void ClockTick() = 0;
protected:
    MemorySystem& memory_system_;
    uint64_t clk_;
};

class RandomCPU : public CPU {
public:
    RandomCPU(MemorySystem& memory_system);
    void ClockTick();
private:
    uint64_t last_addr_;
    bool get_next_ = true;
};

class TraceBasedCPU : public CPU {
public:
    TraceBasedCPU(MemorySystem& memory_system, std::string trace_file);
    void ClockTick();
private:
    std::ifstream trace_file_;
    Access access_;
    bool get_next_ = true;
};

}
#endif