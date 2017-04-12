#ifndef __CPU_H
#define __CPU_H

#include <fstream>
#include <vector>
#include "controller.h"
#include "common.h"
#include "config.h"
#include "memory_system.h"

namespace dramcore {

class RandomCPU {
public:
    RandomCPU(MemorySystem& memory_system, const Config& config);
    void ClockTick();
private:
    MemorySystem& memory_system_;
    const Config& config_;
    long clk_;
    Address last_addr_;
    Request* req_;
    bool get_next_ = true;
    int req_id_ = 0;
    std::ofstream req_log_;
};

class TraceBasedCPU {
public:
    TraceBasedCPU(MemorySystem& memory_system, const Config& config);
    void ClockTick();
private:
    MemorySystem& memory_system_;
    const Config& config_;
    long clk_;
    std::ifstream trace_file_;
    Request* req_;
    bool get_next_ = true;
    int req_id_ = 0;

    Request* FormRequest(const Access& access);
};

}
#endif