#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include <functional>
#include "common.h"
#include "channel_state.h"
#include "command_queue.h"
#include "refresh.h"
#include "statistics.h"
#include "thermal.h"

namespace dramcore {

class Controller {
public:
    Controller(int channel, const Config &config, const Timing &timing, Statistics &stats, ThermalCalculator *thermcalc, std::function<void(uint64_t)>& callback_);
    ~Controller() {};
    void ClockTick();
    bool InsertReq(Request* req);
    std::function<void(uint64_t)>& callback_;
    int channel_id_;
protected:
    uint64_t clk_;
    const Config& config_;
    ChannelState channel_state_;
    CommandQueue cmd_queue_;
    Refresh refresh_;
    Statistics& stats_;
};

}
#endif
