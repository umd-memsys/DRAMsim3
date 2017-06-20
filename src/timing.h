#ifndef __TIMING_H
#define __TIMING_H

#include <vector>
#include <list>
#include "common.h"
#include "configuration.h"

namespace dramcore {

class Timing {
    public:
        Timing(const Config& config);
        const Config& config_; 
        std::vector< std::list< std::pair<CommandType, uint32_t> > > same_bank;
        std::vector< std::list< std::pair<CommandType, uint32_t> > > other_banks_same_bankgroup;
        std::vector< std::list< std::pair<CommandType, uint32_t> > > other_bankgroups_same_rank;
        std::vector< std::list< std::pair<CommandType, uint32_t> > > other_ranks;
        std::vector< std::list< std::pair<CommandType, uint32_t> > > same_rank;

        uint32_t read_to_read_l;
        uint32_t read_to_read_s;
        uint32_t read_to_read_o;
        uint32_t read_to_write;
        uint32_t read_to_write_o;
        uint32_t read_to_precharge;
        uint32_t readp_to_act;

        uint32_t write_to_read_l;
        uint32_t write_to_read_s;
        uint32_t write_to_read_o;
        uint32_t write_to_write_l;
        uint32_t write_to_write_s;
        uint32_t write_to_write_o;
        uint32_t write_to_precharge;

        uint32_t precharge_to_activate;
        uint32_t precharge_to_precharge;
        uint32_t read_to_activate;
        uint32_t write_to_activate;

        uint32_t activate_to_activate;
        uint32_t activate_to_activate_l;
        uint32_t activate_to_activate_s;
        uint32_t activate_to_precharge;
        uint32_t activate_to_read;
        uint32_t activate_to_write;
        uint32_t activate_to_refresh;
        uint32_t refresh_to_refresh;
        uint32_t refresh_to_activate;
        uint32_t refresh_to_activate_bank;

        uint32_t self_refresh_entry_to_exit;
        uint32_t self_refresh_exit;
        uint32_t powerdown_to_exit;
        uint32_t powerdown_exit;

};

}
#endif
