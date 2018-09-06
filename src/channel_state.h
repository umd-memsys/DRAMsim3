#ifndef __CHANNEL_STATE_H
#define __CHANNEL_STATE_H

#include <vector>
#include "bankstate.h"
#include "common.h"
#include "configuration.h"
#include "timing.h"

#ifdef THERMAL
#include "thermal.h"
#endif  // THERMAL

namespace dramsim3 {

class ChannelState {
   public:
    ChannelState(const Config& config, const Timing& timing, Statistics& stats);
    Command GetRequiredCommand(const Command& cmd) const;
    bool IsReady(const Command& cmd, uint64_t clk) const;
    void UpdateState(const Command& cmd);
    void UpdateTiming(const Command& cmd, uint64_t clk);
    void UpdateTimingAndStates(const Command& cmd, uint64_t clk);
    bool ActivationWindowOk(int rank, uint64_t curr_time) const;
    void UpdateActivationTimes(int rank, uint64_t curr_time);
    bool IsRowOpen(int rank, int bankgroup, int bank) const {
        return bank_states_[rank][bankgroup][bank].IsRowOpen();
    }
    bool IsAllBankIdleInRank(int rank) const;
    bool IsRankSelfRefreshing(int rank) const { return is_selfrefresh_[rank]; }
    int OpenRow(int rank, int bankgroup, int bank) const {
        return bank_states_[rank][bankgroup][bank].OpenRow();
    }
    int RowHitCount(int rank, int bankgroup, int bank) const {
        return bank_states_[rank][bankgroup][bank].RowHitCount();
    };

    bool need_to_update_refresh_waiting_status_ = true;
    std::vector<bool> rank_in_self_refresh_mode_;

   private:
    const Config& config_;
    const Timing& timing_;
#ifdef GENERATE_TRACE
    std::ofstream cmd_trace_;
#endif  // GENERATE_TRACE
    Statistics& stats_;
    std::vector<std::vector<std::vector<BankState> > > bank_states_;
    std::vector<std::vector<uint64_t> > four_aw;
    std::vector<std::vector<uint64_t> > thirty_two_aw;
    std::vector<bool> is_selfrefresh_;

    bool IsFAWReady(int rank, uint64_t curr_time) const;
    bool Is32AWReady(int rank, uint64_t curr_time) const;
    // Update timing of the bank the command corresponds to
    void UpdateSameBankTiming(
        const Address& addr,
        const std::vector<std::pair<CommandType, int> >& cmd_timing_list,
        uint64_t clk);

    // Update timing of the other banks in the same bankgroup as the command
    void UpdateOtherBanksSameBankgroupTiming(
        const Address& addr,
        const std::vector<std::pair<CommandType, int> >& cmd_timing_list,
        uint64_t clk);

    // Update timing of banks in the same rank but different bankgroup as the
    // command
    void UpdateOtherBankgroupsSameRankTiming(
        const Address& addr,
        const std::vector<std::pair<CommandType, int> >& cmd_timing_list,
        uint64_t clk);

    // Update timing of banks in a different rank as the command
    void UpdateOtherRanksTiming(
        const Address& addr,
        const std::vector<std::pair<CommandType, int> >& cmd_timing_list,
        uint64_t clk);

    // Update timing of the entire rank (for rank level commands)
    void UpdateSameRankTiming(
        const Address& addr,
        const std::vector<std::pair<CommandType, int> >& cmd_timing_list,
        uint64_t clk);

    void UpdateCommandIssueStats(const Command& cmd) const;
};

}  // namespace dramsim3
#endif
