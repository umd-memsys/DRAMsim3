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
    BaseMemorySystem(std::function<void(uint64_t)> callback) :callback_(callback), clk_(0) {} 
    virtual bool InsertReq(uint64_t req_id, uint64_t hex_addr, bool is_write) {return true;}
    virtual void ClockTick() {}
    virtual void PrintIntermediateStats() {}
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
    std::ofstream address_trace_;
};


class MemorySystem : public BaseMemorySystem {
public:
    MemorySystem(const std::string &config_file, std::function<void(uint64_t)> callback);
    ~MemorySystem();
    bool InsertReq(uint64_t req_id, uint64_t hex_addr, bool is_write);
    void ClockTick();
    void PrintIntermediateStats();

private:
    std::list<Request*> buffer_q_;

};

}

#endif