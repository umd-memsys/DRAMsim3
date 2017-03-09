#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <vector>
#include <list>
#include "common.h"
#include "channel_state.h"

class CommandQueue {
    public:
        CommandQueue(int ranks, int bankgroups, int banks_per_group, const ChannelState& channel_state_);
        Command GetCommandToIssue() ;
        bool InsertReq(Request* req);
        long clk;
    private:
        int ranks_, bankgroups_, banks_per_group_;
        int next_rank_, next_bankgroup_, next_bank_;
        int size_q_;
        std::vector< std::vector< std::vector< std::list<Request*> > > > req_q_;
        const ChannelState& channel_state_;

        void IterateNext();
};


#endif