#ifndef __MEMORY_SYSTEM__H
#define __MEMORY_SYSTEM__H

#include <fstream>
#include <vector>
#include <functional>
#include <list>
#include "common.h"
#include "configuration.h"
#include "timing.h"
#include "statistics.h"
#include "controller.h"
#include "thermal.h"

namespace dramcore {

class BaseMemorySystem {
public:
    BaseMemorySystem(const std::string &config_file, const std::string &output_dir, std::function<void(uint64_t)> read_callback, std::function<void(uint64_t)> write_callback);
    virtual ~BaseMemorySystem();
    virtual bool IsReqInsertable(uint64_t hex_addr, bool is_write) = 0;
    virtual bool InsertReq(uint64_t hex_addr, bool is_write) = 0;
    virtual void ClockTick() = 0;
    virtual void PrintIntermediateStats();
    virtual void PrintStats();
    std::function<void(uint64_t req_id)> read_callback_, write_callback_;
    std::vector<Controller*> ctrls_;
    Config* ptr_config_;
    ThermalCalculator* ptr_thermCal_;
    static int num_mems_;  // a lot of CPU sims create a MemorySystem for each channel, oh well..

protected:
    uint64_t clk_;
    uint64_t id_;
    uint64_t last_req_clk_;
    Timing* ptr_timing_;
    Statistics* ptr_stats_;
    int mem_sys_id_;

    //Stats output files
    std::ofstream stats_file_;
    std::ofstream epoch_stats_file_;
    std::ofstream stats_file_csv_;
    std::ofstream epoch_stats_file_csv_;
    std::ofstream histo_stats_file_csv_;
#ifdef GENERATE_TRACE
    std::ofstream address_trace_;
#endif
};


class MemorySystem : public BaseMemorySystem {
public:
    MemorySystem(const std::string &config_file, const std::string &output_dir, std::function<void(uint64_t)> read_callback, std::function<void(uint64_t)> write_callback);
    ~MemorySystem();
    bool IsReqInsertable(uint64_t hex_addr, bool is_write) override ;
    bool InsertReq(uint64_t hex_addr, bool is_write) override ;
    void ClockTick() override ;
private:
#ifdef NO_BACKPRESSURE
    std::list<Request*> buffer_q_;
#endif
};


// Model a memorysystem with an infinite bandwidth and a fixed latency (possibly zero)
// To establish a baseline for what a 'good' memory standard can and cannot do for a given application
class IdealMemorySystem : public BaseMemorySystem {
public:
    IdealMemorySystem(const std::string &config_file, const std::string &output_dir, std::function<void(uint64_t)> read_callback, std::function<void(uint64_t)> write_callback);
    ~IdealMemorySystem();
    bool IsReqInsertable(uint64_t hex_addr, bool is_write) override { return true; };
    bool InsertReq(uint64_t hex_addr, bool is_write) override ;
    void ClockTick() override ;
private:
    uint32_t latency_;
    std::list<Request*> infinite_buffer_q_;

};


}

#endif