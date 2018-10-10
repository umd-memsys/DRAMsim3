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
    Command GetCommandToIssueFromQueue(std::vector<Command>& queue);
    Command GetFristReadyInQueue(std::vector<Command>& queue);
    CMDIterator GetFirstRWInQueue(CMDQueue& queue);
    Command GetFristReadyInBank(int rank, int bankgroup, int bank);
    void IssueRWCommand(const Command& cmd);
    bool WillAcceptCommand(int rank, int bankgroup, int bank);
    bool AddCommand(Command cmd);
    int QueueUsage() const;
    std::vector<Command>& GetQueue(int rank, int bankgroup, int bank);
    uint64_t clk_;
    std::vector<bool> rank_queues_empty;
    std::vector<uint64_t> rank_queues_empty_from_time_;

   private:
    QueueStructure queue_structure_;
    const Config& config_;
    const ChannelState& channel_state_;
    Statistics& stats_;
    int next_rank_, next_bankgroup_, next_bank_, next_queue_index_;
    std::vector<std::vector<Command>> queues_;
    size_t queue_size_;
    int channel_id_;

    void IterateNext();
    int GetQueueIndex(int rank, int bankgroup, int bank);
};

}  // namespace dramsim3
#endif