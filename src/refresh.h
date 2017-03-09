#ifndef __REFRESH_H
#define __REFRESH_H

#include <vector>
#include <list>
#include "common.h"

class Refresh {
    public:
        Refresh(int ranks, int bankgroups, int banks_per_group);
        std::list<Request*> refresh_q_;
        bool PrepareForRefreshIssue();
    private:
        long clk;
        int ranks_, bankgroups_, banks_per_group_;
        // Queue of refresh commands

        //Keep track of the last time when a refresh command was issued to this bank 
        std::vector< std::vector< std::vector<long> > > last_bank_refresh_;

        //Last time when a refresh command was issued to the entire rank
        //Also updated when an epoch of bank level refreshed is done as well
        std::vector<long> last_rank_refresh_;

        int next_rank_;

        void ClockTick();

        void InsertRefresh();

        void IterateNext();

        int tREFI = 7800;
        int tREFIb = 1950; //4 banks. Temporary hard coding.
        
};

#endif