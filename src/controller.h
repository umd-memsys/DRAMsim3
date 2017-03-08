#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <vector>
#include "common.h"
#include "bankstate.h"
#include "timing.h"

class Controller {
    public:
        Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing);
        Command GetRequiredCommand(const Command& cmd);
        bool IsReady(const Command& cmd);
        void UpdateState(const Command& cmd);
        void UpdateTiming(const Command& cmd);
    private:
        long clk;
        const Timing& timing_;
        int ranks_, bankgroups_, banks_per_group_;
        std::vector< std::vector< std::vector<BankState*> > > bank_states_;

        //Update timing of the bank the command corresponds to
        void UpdateSameBankTiming(int rank, int bankgroup, int bank, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of the other banks in the same bankgroup as the command
        void UpdateOtherBanksSameBankgroupTiming(int rank, int bankgroup, int bank, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of banks in the same rank but different bankgroup as the command
        void UpdateOtherBankgroupsSameRankTiming(int rank, int bankgroup, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of banks in a different rank as the command
        void UpdateOtherRanksTiming(int rank, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

        //Update timing of the entire rank (for rank level commands)
        void UpdateSameRankTiming(int rank, const std::list< std::pair<CommandType, int> >& cmd_timing_list);

};

#endif