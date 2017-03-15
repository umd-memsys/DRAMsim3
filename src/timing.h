#ifndef __TIMING_H
#define __TIMING_H

#include <vector>
#include <list>
#include "common.h"
#include "config.h"


class Timing {
    public:
        Timing(const Config& config);
        const Config& config_; 
        std::vector< std::list< std::pair<CommandType, int> > > same_bank;
        std::vector< std::list< std::pair<CommandType, int> > > other_banks_same_bankgroup;
        std::vector< std::list< std::pair<CommandType, int> > > other_bankgroups_same_rank;
        std::vector< std::list< std::pair<CommandType, int> > > other_ranks;
        std::vector< std::list< std::pair<CommandType, int> > > same_rank;

        int read_to_read_l;
        int read_to_read_s;
        int read_to_read_o;
        int read_to_write;
        int read_to_write_o;
        int read_to_precharge;

        int write_to_read;
        int write_to_read_o;
        int write_to_write_l;
        int write_to_write_s;
        int write_to_write_o;
        int write_to_precharge;

        int precharge_to_activate;
        int read_to_activate;
        int write_to_activate;

        int activate_to_activate;
        int activate_to_activate_o;
        int activate_to_precharge;
        int activate_to_read_write;
        int activate_to_refresh;
        int refresh_to_refresh;
        int refresh_to_activate;

        int refresh_cycle;

        int refresh_cycle_bank;

        int self_refresh_entry_to_exit;
        int self_refresh_exit;
};

#endif
