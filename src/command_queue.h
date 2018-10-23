#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <functional>
#include <vector>
#include "channel_state.h"
#include "common.h"
#include "configuration.h"
#include "statistics.h"

namespace dramsim3 {

using CMDIterator = std::vector<Command>::iterator;
using CMDQueue = std::vector<Command>;
enum class QueueStructure { PER_RANK, PER_BANK, SIZE };

class CommandQueue {
   public:
    CommandQueue(int channel_id, const Config& config,
                 const ChannelState& channel_state, Statistics& stats);
    Command GetCommandToIssue();
    void ClockTick() { clk_ += 1; };
    void IssueRWCommand(const Command& cmd);
    bool WillAcceptCommand(int rank, int bankgroup, int bank);
    bool AddCommand(Command cmd);
    int QueueUsage() const;
    std::vector<bool> rank_q_empty;

   private:
    QueueStructure queue_structure_;
    const Config& config_;
    const ChannelState& channel_state_;
    Statistics& stats_;
    int next_rank_, next_bg_, next_bank_;
    std::vector<CMDQueue> queues_;
    size_t queue_size_;
    uint64_t clk_;

    bool ArbitratePrecharge(const CMDIterator& cmd_it,
                            const CMDQueue& queue) const;
    Command GetFristReadyInQueue(CMDQueue& queue);
    CMDQueue& GetQueue(int rank, int bankgroup, int bank);
    int GetQueueIndex(int rank, int bankgroup, int bank);
    CMDQueue& GetNextQueue();
    bool HasRWDependency(const CMDIterator& cmd_it,
                         const CMDQueue& queue) const;
    Command PrepRefCmd(CMDIterator& it, Command& ref);
};

}  // namespace dramsim3
#endif