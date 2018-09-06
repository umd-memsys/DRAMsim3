#ifndef __REFRESH_H
#define __REFRESH_H

#include <vector>
#include "channel_state.h"
#include "command_queue.h"
#include "common.h"
#include "configuration.h"

namespace dramsim3 {

enum class RefreshStrategy {
    RANK_LEVEL_SIMULTANEOUS,  // impractical due to high power requirement
    RANK_LEVEL_STAGGERED,
    BANK_LEVEL_STAGGERED,
    UNKNOWN
};

class Refresh {
   public:
    Refresh(const Config& config,
            const ChannelState& channel_state, CommandQueue& cmd_queue,
            Statistics& stats);
    std::vector<Command> refresh_q_;  // Queue of refresh commands
    void ClockTick();
    bool IsRefWaiting();
    void RefreshIssued(const Command& cmd);
    Command GetRefreshOrAssociatedCommand();

   private:
    uint64_t clk_;
    int refresh_interval_;
    std::vector<std::vector<std::vector<bool> > > refresh_waiting_;
    const Config& config_;
    const ChannelState& channel_state_;
    CommandQueue& cmd_queue_;
    Statistics& stats_;
    RefreshStrategy refresh_strategy_;

    int next_rank_, next_bankgroup_, next_bank_;

    void UpdateRefWaiting(const Command& cmd, bool status);
    void InsertRefresh();

    void IterateNext();

    bool ReadWritesToFinish(int rank, int bankgroup, int bank);
    Command GetReadWritesToOpenRow(int rank, int bankgroup, int bank);
};

}  // namespace dramsim3

#endif