#ifndef __CPU_H
#define __CPU_H

#include <fstream>
#include <random>
#include <vector>
#include "common.h"
#include "configuration.h"
#include "controller.h"
#include "memory_system.h"

namespace dramsim3 {

class CPU {
   public:
    CPU(MemorySystem& memory_system) : memory_system_(memory_system), clk_(0) {}
    virtual ~CPU() { memory_system_.PrintStats(); }
    virtual void ClockTick() = 0;

   protected:
    MemorySystem& memory_system_;
    uint64_t clk_;
};

class RandomCPU : public CPU {
   public:
    RandomCPU(MemorySystem& memory_system) : CPU(memory_system) {}
    void ClockTick() override;

   private:
    uint64_t last_addr_;
    std::mt19937_64 gen;
    bool get_next_ = true;
};

class StreamCPU : public CPU {
   public:
    StreamCPU(MemorySystem& memory_system) : CPU(memory_system) {}
    void ClockTick() override;

   private:
    uint64_t addr_a_, addr_b_, addr_c_, offset_ = 0;
    std::mt19937_64 gen;
    bool inserted_a_ = false;
    bool inserted_b_ = false;
    bool inserted_c_ = false;
    const uint64_t array_size_ = 2 << 20;  // elements in array
    const int stride_ = 64;                // stride in bytes
};

class TraceBasedCPU : public CPU {
   public:
    TraceBasedCPU(MemorySystem& memory_system, std::string trace_file);
    ~TraceBasedCPU() { trace_file_.close(); }
    void ClockTick() override;

   private:
    std::ifstream trace_file_;
    Access access_;
    bool get_next_ = true;
};

}  // namespace dramsim3
#endif
