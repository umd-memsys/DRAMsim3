#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <unordered_set>
#include <vector>
#include "channel_state.h"
#include "common.h"
#include "configuration.h"
#include "simple_stats.h"

namespace dramsim3 {

using CMDIterator = std::vector<Command>::iterator;
using CMDQueue = std::vector<Command>;
enum class QueueStructure { PER_RANK, PER_BANK, SIZE };

class CommandQueue {
   public:
    CommandQueue(int channel_id, const Config& config,
                 const ChannelState& channel_state, SimpleStats& simple_stats);
    Command GetCommandToIssue();
    void ClockTick() { clk_ += 1; };
    bool WillAcceptCommand(int rank, int bankgroup, int bank) const;
    bool AddCommand(Command cmd);
    int QueueUsage() const;
    std::vector<bool> rank_q_empty;

   private:
    QueueStructure queue_structure_;
    const Config& config_;
    const ChannelState& channel_state_;
    SimpleStats& simple_stats_;
    std::vector<CMDQueue> queues_;
    int num_queues_;
    size_t queue_size_;
    int queue_idx_;
    uint64_t clk_;

    bool ArbitratePrecharge(const CMDIterator& cmd_it,
                            const CMDQueue& queue) const;
    Command GetFristReadyInQueue(CMDQueue& queue) const;
    CMDQueue& GetQueue(int rank, int bankgroup, int bank);
    int GetQueueIndex(int rank, int bankgroup, int bank) const;
    std::unordered_set<int> GetRefQIndices(const Command& ref) const;
    CMDQueue& GetNextQueue();
    bool HasRWDependency(const CMDIterator& cmd_it,
                         const CMDQueue& queue) const;
    void EraseRWCommand(const Command& cmd);
    Command PrepRefCmd(const CMDIterator& it, const Command& ref) const;
};

}  // namespace dramsim3
#endif