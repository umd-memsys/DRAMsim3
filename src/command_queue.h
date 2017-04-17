#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <vector>
#include <list>
#include <functional>
#include "common.h"
#include "channel_state.h"
#include "config.h"
#include "statistics.h"

namespace dramcore {

class CommandQueue {
public:
    CommandQueue(const Config &config, const ChannelState &channel_state, Statistics &stats, std::function<void(uint64_t)>& callback);
    Command GetCommandToIssue();
    Command GetCommandToIssueFromQueue(std::list<Request*>& queue);
    Command AggressivePrecharge();
    std::function<void(uint64_t)>& callback_;
    void IssueRequest(std::list<Request*>& queue, std::list<Request*>::iterator req_itr);
    bool InsertReq(Request* req);
    std::list<Request*>& GetQueue(int rank, int bankgroup, int bank);
    uint64_t clk_;
    std::list<Request*> issued_req_; //TODO - Here or in the controller or main?
private:
    const Config& config_;
    const ChannelState& channel_state_;
    Statistics& stats_;
    int next_rank_, next_bankgroup_, next_bank_;
    std::vector< std::vector< std::vector< std::list<Request*> > > > req_q_per_bank_;
    std::vector< std::list<Request*> > req_q_per_rank_;

    void IterateNext();
};

}
#endif