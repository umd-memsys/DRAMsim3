#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <functional>
#include <list>
#include <vector>
#include "channel_state.h"
#include "common.h"
#include "configuration.h"
#include "statistics.h"

namespace dramsim3 {

enum class QueueStructure { PER_RANK, PER_BANK, SIZE };

class CommandQueue {
   public:
    CommandQueue(int channel_id, const Config& config,
                 const ChannelState& channel_state, Statistics& stats);
    Command GetCommandToIssue();
    Command GetCommandToIssueFromQueue(std::list<Command>& queue);
    Command AggressivePrecharge();
    // void IssueRequest(std::list<Request*>& queue,
    //                   std::list<Request*>::iterator req_itr);
    void IssueCommand(std::list<Command>& queue, 
                      std::list<Command>::iterator it);
    bool WillAcceptCommand(int rank, int bankgroup, int bank);
    bool AddCommand(Command cmd);
    // bool IsReqInsertable(Request* req);
    // bool InsertReq(Request* req);
    int QueueUsage() const;
    std::list<Command>& GetQueue(int rank, int bankgroup, int bank);
    uint64_t clk_;
    // std::list<Request*> issued_req_;
    std::vector<bool> rank_queues_empty;
    std::vector<uint64_t> rank_queues_empty_from_time_;

   private:
    QueueStructure queue_structure_;
    const Config& config_;
    const ChannelState& channel_state_;
    Statistics& stats_;
    int next_rank_, next_bankgroup_, next_bank_, next_queue_index_;
    std::vector<std::list<Command>> queues_;
    size_t queue_size_;
    int channel_id_;

    void IterateNext();
    int GetQueueIndex(int rank, int bankgroup, int bank);
};

}  // namespace dramsim3
#endif