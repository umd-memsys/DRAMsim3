#ifndef __CHANNEL_STATE_H
#define __CHANNEL_STATE_H

#include "common.h"
#include "bankstate.h"
#include "timing.h"
#include "config.h"
#include <vector>
#include <list>

class ChannelState {
    public:
        ChannelState(const Config& config, const Timing& timing);
        Command GetRequiredCommand(const Command& cmd) const;
        bool IsReady(const Command& cmd, long clk) const;
        void UpdateState(const Command& cmd);
        void UpdateTiming(const Command& cmd, long clk);
        void IssueCommand(const Command& cmd, long clk);
        void UpdateRefreshWaitingStatus(const Command& cmd, bool status);
        bool ActivationConstraint(int rank, long curr_time) const;
        void UpdateActivationTimes(int rank, long curr_time);
        bool IsRefreshWaiting(int rank, int bankgroup, int bank) const { return bank_states_[rank][bankgroup][bank]->IsRefreshWaiting(); }
        bool IsRowOpen(int rank, int bankgroup, int bank) const { return bank_states_[rank][bankgroup][bank]->IsRowOpen(); }
        int OpenRow(int rank, int bankgroup, int bank) const { return bank_states_[rank][bankgroup][bank]->OpenRow(); }
        int RowHitCount(int rank, int bankgroup, int bank) const { return bank_states_[rank][bankgroup][bank]->RowHitCount(); };
    private:
        const Config& config_;
        const Timing& timing_;
        std::vector< std::vector< std::vector<BankState*> > > bank_states_;
        std::vector< std::list<long> > activation_times_;

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