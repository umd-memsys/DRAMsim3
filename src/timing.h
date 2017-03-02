#ifndef __TIMING_H
#define __TIMING_H

#include <vector>
#include <list>
#include <utility>
#include "bankstate.h"

class Timing {
    public:
        Timing();
        std::vector< std::list< std::pair<CommandType, int> > > same_bank;
        std::vector< std::list< std::pair<CommandType, int> > > other_banks_same_bankgroup;
        std::vector< std::list< std::pair<CommandType, int> > > other_bankgroups_same_rank;
        std::vector< std::list< std::pair<CommandType, int> > > other_ranks;

        int tBurst = 4;
        int tCCDL = 6;
        int tCCDS = 5;
        int tRTRS = 2;
        int tRTP = 6;
};

#endif