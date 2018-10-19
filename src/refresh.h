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
    Refresh(const Config& config, ChannelState& channel_state);
    void ClockTick();

   private:
    uint64_t clk_;
    int refresh_interval_;
    const Config& config_;
    ChannelState& channel_state_;
    RefreshStrategy refresh_strategy_;

    int next_rank_, next_bg_, next_bank_;

    void InsertRefresh();

    void IterateNext();
};

}  // namespace dramsim3

#endif