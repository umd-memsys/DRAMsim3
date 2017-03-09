#ifndef __CHANNEL_STATE_H
#define __CHANNEL_STATE_H

#include "common.h"
#include "bankstate.h"
#include "timing.h"
#include <vector>
#include <list>

class ChannelState {
    public:
        ChannelState(int ranks, int bankgroups, int banks_per_group, const Timing& timing);
        Command GetRequiredCommand(const Command& cmd) const;
        bool IsReady(const Command& cmd, long clk) const;
        void UpdateState(const Command& cmd);
        void UpdateTiming(const Command& cmd, long clk);

    private:
        int ranks_, bankgroups_, banks_per_group_;
        std::vector< std::vector< std::vector<BankState*> > > bank_states_;
        const Timing& timing_;

        //Update timing of the bank the command corresponds to
        void UpdateSameBankTiming(int rank, int bankgroup, int bank, const std::list< std::pair<CommandType, int> >& cmd_timing_list, long clk);

        //Update timing of the other banks in the same bankgroup as the command
        void UpdateOtherBanksSameBankgroupTiming(int rank, int bankgroup, int bank, const std::list< std::pair<CommandType, int> >& cmd_timing_list, long clk);

        //Update timing of banks in the same rank but different bankgroup as the command
        void UpdateOtherBankgroupsSameRankTiming(int rank, int bankgroup, const std::list< std::pair<CommandType, int> >& cmd_timing_list, long clk);

        //Update timing of banks in a different rank as the command
        void UpdateOtherRanksTiming(int rank, const std::list< std::pair<CommandType, int> >& cmd_timing_list, long clk);

        //Update timing of the entire rank (for rank level commands)
        void UpdateSameRankTiming(int rank, const std::list< std::pair<CommandType, int> >& cmd_timing_list, long clk);
};

#endif