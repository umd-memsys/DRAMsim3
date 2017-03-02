#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include "bankstate.h"

class Controller {
    public:
        Controller(int ranks, int bankgroups, int banks_per_group);
        void UpdateTiming(const Command& cmd);
    private:
        long clk;
        int ranks_, bankgroups_, banks_per_group_;
        std::vector< std::vector< std::vector<BankState*> > > bank_states_;

        //Update timing of the bank the command corresponds to
        void UpdateSameBank(const Command& cmd, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of the other banks in the same bankgroup as the command
        void UpdateOtherBanksSameBankgroup(const Command& cmd, CommandType cmd_type, long time);

        /*
        //Update all banks of the same bankgroup as the command
        void UpdateSameBankgroup(const Command& cmd, CommandType cmd_type, long time);
        */

        //Update timing of banks in the same rank but different bankgroup as the command
        void UpdateOtherBankgroups(const Command& cmd, CommandType cmd_type, long time);

        //Update timing of banks in a different rank as the command
        void UpdateOtherRanks(const Command& cmd, CommandType cmd_type, long time);

        int tBurst = 4;
        int tCCDL = 6;
        int tCCDS = 5;
        int tRTRS = 2;
        int tRTP = 6;


};

#endif