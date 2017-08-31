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
    Controller(int channel, const Config &config, const Timing &timing, Statistics &stats, std::function<void(uint64_t)> read_callback, std::function<void(uint64_t)> write_callback);
    ~Controller() {};
    void ClockTick();
    bool IsReqInsertable(Request* req);
    bool InsertReq(Request* req);
    std::function<void(uint64_t)> read_callback_, write_callback_;
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
