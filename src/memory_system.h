#ifndef __MEMORY_SYSTEM__H
#define __MEMORY_SYSTEM__H

#include <fstream>
#include <vector>
#include <functional>
#include "common.h"
#include "config.h"
#include "timing.h"
#include "statistics.h"
#include "controller.h"

namespace dramcore {

class MemorySystem {
public:
    MemorySystem(std::function<void(uint64_t)> callback);
    bool InsertReq(uint64_t req_id, uint64_t hex_addr_, CommandType cmd_type);
    void ClockTick();
    void PrintStats();
    std::function<void(uint64_t req_id)> callback_;
    std::vector<Controller*> ctrls_;
    Config config_;
private:
    Timing timing_;
    Statistics stats_;
};

}

#endif