#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <vector>
#include <list>
#include <functional>
#include "common.h"
#include "channel_state.h"
#include "configuration.h"
#include "statistics.h"

namespace dramcore {

enum class QueueStructure {
    PER_RANK,
    PER_BANK,
    SIZE
};

class CommandQueue {
public:
    CommandQueue(uint32_t channel_id, const Config &config, const ChannelState &channel_state, Statistics &stats);
    Command GetCommandToIssue();
    Command GetCommandToIssueFromQueue(std::list<Request*>& queue);
    Command AggressivePrecharge();
    void IssueRequest(std::list<Request*>& queue, std::list<Request*>::iterator req_itr);
    bool IsReqInsertable(Request* req);
    bool InsertReq(Request* req);
    int QueueUsage() const;
    std::list<Request*>& GetQueue(int rank, int bankgroup, int bank);
    uint64_t clk_;
    std::list<Request*> issued_req_; //TODO - Here or in the controller or main?
    std::vector<bool> rank_queues_empty;
    std::vector<uint64_t > rank_queues_empty_from_time_;
private:
    QueueStructure queue_structure_;
    const Config& config_;
    const ChannelState& channel_state_;
    Statistics& stats_;
    int next_rank_, next_bankgroup_, next_bank_, next_queue_index_;
    std::vector<std::list<Request*>> queues_;
    uint32_t channel_id_;

    void IterateNext();
    int GetQueueIndex(int rank, int bankgroup, int bank);
};

}
#endif