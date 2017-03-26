#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <vector>
#include <list>
#include "common.h"
#include "channel_state.h"
#include "config.h"

class CommandQueue {
    public:
        CommandQueue(const Config& config, const ChannelState& channel_state);
        Command GetCommandToIssue();
        Command GetCommandToIssueFromQueue(std::list<Request*>& queue);
        Command AggressivePrecharge();
        bool InsertReq(Request* req);
        std::list<Request*>& GetQueue(int rank, int bankgroup, int bank);
        long clk;
    private:
        const Config& config_;
        const ChannelState& channel_state_;
        int next_rank_, next_bankgroup_, next_bank_;
        std::vector< std::vector< std::vector< std::list<Request*> > > > req_q_per_bank_;
        std::vector< std::list<Request*> > req_q_per_rank_;

        void IterateNext();
};


#endif