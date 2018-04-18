#ifndef __TIMING_H
#define __TIMING_H

#include <list>
#include <vector>
#include "common.h"
#include "configuration.h"

namespace dramcore {

class Timing {
   public:
    Timing(const Config& config);
    std::vector<std::list<std::pair<CommandType, uint32_t> > > same_bank;
    std::vector<std::list<std::pair<CommandType, uint32_t> > >
        other_banks_same_bankgroup;
    std::vector<std::list<std::pair<CommandType, uint32_t> > >
        other_bankgroups_same_rank;
    std::vector<std::list<std::pair<CommandType, uint32_t> > > other_ranks;
    std::vector<std::list<std::pair<CommandType, uint32_t> > > same_rank;
};

}  // namespace dramcore
#endif
