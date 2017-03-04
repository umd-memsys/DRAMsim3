#ifndef __TIMING_H
#define __TIMING_H

#include <vector>
#include <list>
#include <utility>
#include "common.h"

class Timing {
    public:
        Timing();
        std::vector< std::list< std::pair<CommandType, int> > > same_bank;
        std::vector< std::list< std::pair<CommandType, int> > > other_banks_same_bankgroup;
        std::vector< std::list< std::pair<CommandType, int> > > other_bankgroups_same_rank;
        std::vector< std::list< std::pair<CommandType, int> > > other_ranks;
        std::vector< std::list< std::pair<CommandType, int> > > same_rank;

        int tBurst = 4;
        int tCCDL = 6;
        int tCCDS = 5;
        int tRTRS = 2;
        int tRTP = 6;
        int tCAS = 3;
        int tCWD = 4;
        int tWTR = 2;
        int tWR = 4;
        int tRP = 5;
        int tRRD = 10;
        int tRAS = 8;
        int tRCD = 4;
        int tRFC = 60;
        int tRC = tRAS + tRP;
        int tCKESR = 50;
        int tXS = 10;
        int tRFCb = 20;
        int tRREFD = 5;

        int read_to_read_l = std::max(tBurst, tCCDL);
        int read_to_read_s = std::max(tBurst, tCCDS);
        int read_to_read_o = tBurst + tRTRS;
        int read_to_write = std::max(tBurst, tCCDL) + tCAS - tCWD; // What if (tCAS - tCWD) < 0? Would that help issue an early write?
        int read_to_write_o = tBurst + tRTRS + tCAS - tCWD; // What if (tCAS - tCWD) < 0? Would that help issue an early write?
        int read_to_precharge = tRTP + tBurst - tCCDL; // What if (tBurst - tCCD) < 0? Would that help issue an early precharge?

        int write_to_read = tCWD + tBurst + tWTR; //Why doesn't tCCD come into the picture?
        int write_to_read_o = tCWD + tBurst + tRTRS - tCAS; // What if (tCWD - tCAS) < 0? Would that help issue an early read?
        int write_to_write_l = std::max(tBurst, tCCDL);
        int write_to_write_s = std::max(tBurst, tCCDS);
        int write_to_write_o = tBurst + tRTRS; // Let's say tRTRS == tOST
        int write_to_precharge = tCWD + tBurst + tWR;

        int precharge_to_activate = tRP;
        int read_to_activate = read_to_precharge + precharge_to_activate;
        int write_to_activate = write_to_precharge + precharge_to_activate;

        int activate_to_activate = tRC;
        int activate_to_activate_o = tRRD;
        int activate_to_precharge = tRAS;
        int activate_to_read_write = tRCD;
        int activate_to_refresh = tRRD;
        int refresh_to_refresh = tRREFD;
        int refresh_to_activate = refresh_to_refresh;

        int refresh_cycle = tRFC;

        int refresh_cycle_bank = tRFCb;

        int self_refresh_entry_to_exit = tCKESR;
        int self_refresh_exit = tXS;



};

#endif