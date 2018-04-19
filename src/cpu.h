#ifndef __CPU_H
#define __CPU_H

#include <fstream>
#include <random>
#include <vector>
#include "common.h"
#include "configuration.h"
#include "controller.h"
#include "dram_system.h"

namespace dramcore {

class CPU {
   public:
    CPU(BaseDRAMSystem& memory_system);
    virtual void ClockTick() = 0;

   protected:
    BaseDRAMSystem& memory_system_;
    uint64_t clk_;
};

class RandomCPU : public CPU {
   public:
    RandomCPU(BaseDRAMSystem& memory_system);
    void ClockTick() override;

   private:
    uint64_t last_addr_;
    std::mt19937_64 gen;
    bool get_next_ = true;
};

class StreamCPU : public CPU {
   public:
    StreamCPU(BaseDRAMSystem& memory_system);
    void ClockTick() override;

   private:
    uint64_t addr_a_, addr_b_, addr_c_, offset_ = 0;
    std::mt19937_64 gen;
    bool get_next_ = true;
    bool inserted_a_, inserted_b_, inserted_c_;
    const uint64_t array_size_ = 2 << 20;  // elements in array
    const int stride_ = 64;                // stride in bytes
};

class TraceBasedCPU : public CPU {
   public:
    TraceBasedCPU(BaseDRAMSystem& memory_system, std::string trace_file);
    void ClockTick() override;

   private:
    std::ifstream trace_file_;
    Access access_;
    bool get_next_ = true;
};

}  // namespace dramcore
#endif
