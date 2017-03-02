#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include "bankstate.h"
#include "timing.h"

class Controller {
    public:
        Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing);
        void UpdateTiming(const Command& cmd);
        const Timing& timing_;
    private:
        long clk;
        int ranks_, bankgroups_, banks_per_group_;
        std::vector< std::vector< std::vector<BankState*> > > bank_states_;

        //Update timing of the bank the command corresponds to
        void UpdateSameBank(const Command& cmd, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of the other banks in the same bankgroup as the command
        void UpdateOtherBanksSameBankgroup(const Command& cmd, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of banks in the same rank but different bankgroup as the command
        void UpdateOtherBankgroupsSameRank(const Command& cmd, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of banks in a different rank as the command
        void UpdateOtherRanks(const Command& cmd, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

};

#endif