#ifndef __REFRESH_H
#define __REFRESH_H

#include <vector>
#include <list>
#include "common.h"
#include "channel_state.h"
#include "command_queue.h"
#include "configuration.h"

namespace dramcore {

enum class RefreshStrategy {
    RANK_LEVEL_SIMULTANEOUS, //impractical due to high power requirement
    RANK_LEVEL_STAGGERED,
    BANK_LEVEL_SIMULTANEOUS,
    BANK_LEVEL_STAGGERED,
    UNKNOWN
};

class Refresh {
public:
    Refresh(const uint32_t channel_id, const Config &config, const ChannelState &channel_state, CommandQueue &cmd_queue, Statistics &stats);
    std::list<Request*> refresh_q_; // Queue of refresh commands
    void ClockTick();
    Command GetRefreshOrAssociatedCommand(std::list<Request*>::iterator refresh_itr);
private:
    uint64_t clk_;
    uint32_t channel_id_;
    const Config& config_;
    const ChannelState& channel_state_;
    CommandQueue& cmd_queue_;
    Statistics& stats_;

    RefreshStrategy refresh_strategy_;

    //Keep track of the last time when a refresh command was issued to this bank
    std::vector< std::vector< std::vector<uint64_t> > > last_bank_refresh_; //TODO - Wouldn't it be better to move this to bankstate?

    //Last time when a refresh command was issued to the entire rank
    //Also updated when an epoch of bank level refreshed is done as well
    std::vector<uint64_t> last_rank_refresh_;

    int next_rank_, next_bankgroup_, next_bank_;

    void InsertRefresh();

    void IterateNext();

    bool ReadWritesToFinish(int rank, int bankgroup, int bank);
    Command GetReadWritesToOpenRow(uint32_t rank, uint32_t bankgroup, uint32_t bank);

};

}

#endif