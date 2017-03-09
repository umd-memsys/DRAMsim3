#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include "common.h"
#include "channel_state.h"
#include "command_queue.h"
#include "refresh.h"

class Controller {
    public:
        Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing);
        void ClockTick();
        bool InsertReq(Request* req);
    private:
        int ranks_, bankgroups_, banks_per_group_;
        long clk;
        ChannelState channel_state_;
        CommandQueue cmd_queue_;
        Refresh refresh_;

        

};

#endif