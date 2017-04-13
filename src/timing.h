#ifndef __TIMING_H
#define __TIMING_H

#include <vector>
#include <list>
#include "common.h"
#include "config.h"

namespace dramcore {

class Timing {
    public:
        Timing(const Config& config);
        const Config& config_; 
        std::vector< std::list< std::pair<CommandType, unsigned int> > > same_bank;
        std::vector< std::list< std::pair<CommandType, unsigned int> > > other_banks_same_bankgroup;
        std::vector< std::list< std::pair<CommandType, unsigned int> > > other_bankgroups_same_rank;
        std::vector< std::list< std::pair<CommandType, unsigned int> > > other_ranks;
        std::vector< std::list< std::pair<CommandType, unsigned int> > > same_rank;

        unsigned int read_to_read_l;
        unsigned int read_to_read_s;
        unsigned int read_to_read_o;
        unsigned int read_to_write;
        unsigned int read_to_write_o;
        unsigned int read_to_precharge;

        unsigned int write_to_read;
        unsigned int write_to_read_o;
        unsigned int write_to_write_l;
        unsigned int write_to_write_s;
        unsigned int write_to_write_o;
        unsigned int write_to_precharge;

        unsigned int precharge_to_activate;
        unsigned int read_to_activate;
        unsigned int write_to_activate;

        unsigned int activate_to_activate;
        unsigned int activate_to_activate_o;
        unsigned int activate_to_precharge;
        unsigned int activate_to_read_write;
        unsigned int activate_to_refresh;
        unsigned int refresh_to_refresh;
        unsigned int refresh_to_activate;

        unsigned int refresh_cycle;

        unsigned int refresh_cycle_bank;

        unsigned int self_refresh_entry_to_exit;
        unsigned int self_refresh_exit;
};

}
#endif
