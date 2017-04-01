#ifndef __REFRESH_H
#define __REFRESH_H

#include <vector>
#include <list>
#include "common.h"
#include "channel_state.h"
#include "command_queue.h"
#include "config.h"

class Refresh {
    public:
        Refresh(const Config& config, const ChannelState& channel_state, CommandQueue& cmd_queue);
        std::list<Request*> refresh_q_; // Queue of refresh commands
        void ClockTick();
        Command GetRefreshOrAssociatedCommand(std::list<Request*>::iterator refresh_itr);
    private:
        const Config& config_;
        const ChannelState& channel_state_;
        CommandQueue& cmd_queue_;
        long clk_;
        
        //Keep track of the last time when a refresh command was issued to this bank 
        std::vector< std::vector< std::vector<long> > > last_bank_refresh_; //TODO - Wouldn't it be better to move this to bankstate?

        //Last time when a refresh command was issued to the entire rank
        //Also updated when an epoch of bank level refreshed is done as well
        std::vector<long> last_rank_refresh_;

        int next_rank_;

        void InsertRefresh();

        void IterateNext();

        bool ReadWritesToFinish(int rank, int bankgroup, int bank);
        Command GetReadWritesToOpenRow(int rank, int bankgroup, int bank);
        
};

#endif