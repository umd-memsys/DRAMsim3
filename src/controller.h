#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include "common.h"
#include "channel_state.h"
#include "command_queue.h"
#include "refresh.h"

class Controller {
    public:
        Controller(int channel, const Config& config, const Timing& timing);
        void ClockTick();
        bool InsertReq(Request* req);
        int channel_;
    private:
        long clk;
        ChannelState channel_state_;
        CommandQueue cmd_queue_;
        Refresh refresh_;
};

#endif