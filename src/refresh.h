#ifndef __REFRESH_H
#define __REFRESH_H

#include <vector>
#include <list>
#include "common.h"
#include "channel_state.h"
#include "command_queue.h"

class Refresh {
    public:
        Refresh(int ranks, int bankgroups, int banks_per_group, const ChannelState& channel_state, CommandQueue& cmd_queue);
        std::list<Request*> refresh_q_;
        void ClockTick();
        Command GetRefreshOrAssociatedCommand(std::list<Request*>::iterator itr);
    private:
        long clk;
        int ranks_, bankgroups_, banks_per_group_;
        const ChannelState& channel_state_;
        CommandQueue& cmd_queue_;
        // Queue of refresh commands

        //Keep track of the last time when a refresh command was issued to this bank 
        std::vector< std::vector< std::vector<long> > > last_bank_refresh_;

        //Last time when a refresh command was issued to the entire rank
        //Also updated when an epoch of bank level refreshed is done as well
        std::vector<long> last_rank_refresh_;

        int next_rank_;

        void InsertRefresh();

        void IterateNext();

        int tREFI = 7800;
        //int tREFIb = 1950; //4 banks. Temporary hard coding.
        
};

#endif