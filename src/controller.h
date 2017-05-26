#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include <functional>
#include "common.h"
#include "channel_state.h"
#include "command_queue.h"
#include "refresh.h"
#include "statistics.h"

namespace dramcore {

class Controller {
public:
    Controller(int channel, const Config &config, const Timing &timing, Statistics &stats, std::function<void(uint64_t)>& callback_);
    ~Controller();
    void ClockTick();
    bool InsertReq(Request* req);
    std::function<void(uint64_t)>& callback_;
    int channel_;
private:
    uint64_t clk_;
    bool val_output_enable;
    std::ofstream val_output_;
    ChannelState channel_state_;
    CommandQueue cmd_queue_;
    Refresh refresh_;
    Statistics& stats_;
};

}
#endif
