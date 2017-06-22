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

namespace dramcore {

class BaseMemorySystem {
public:
    BaseMemorySystem(const std::string &config_file, std::function<void(uint64_t)> callback);
    ~BaseMemorySystem();
    virtual bool InsertReq(uint64_t hex_addr, bool is_write) = 0;
    virtual void ClockTick() = 0;
    virtual void PrintIntermediateStats();
    virtual void PrintStats();
    std::function<void(uint64_t req_id)> callback_;
    std::vector<Controller*> ctrls_;
    Config* ptr_config_;

protected:
    uint64_t clk_;
    uint64_t id_;
    Timing* ptr_timing_;
    Statistics* ptr_stats_;

    //Stats output files
    std::ofstream stats_file_;
    std::ofstream cummulative_stats_file_;
    std::ofstream epoch_stats_file_;
    std::ofstream stats_file_csv_;
    std::ofstream cummulative_stats_file_csv_;
    std::ofstream epoch_stats_file_csv_;
#ifdef GENERATE_TRACE
    std::ofstream address_trace_;
#endif
};


class MemorySystem : public BaseMemorySystem {
public:
    MemorySystem(const std::string &config_file, std::function<void(uint64_t)> callback);
    ~MemorySystem();
    bool InsertReq(uint64_t hex_addr, bool is_write);
    void ClockTick();
private:
    std::list<Request*> buffer_q_;

};

}

#endif