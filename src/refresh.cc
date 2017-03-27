#include "refresh.h"

using namespace std;

Refresh::Refresh(const Config& config, const ChannelState& channel_state, CommandQueue& cmd_queue) :
    config_(config),
    channel_state_(channel_state),
    cmd_queue_(cmd_queue),
    clk(0),
    last_bank_refresh_(config_.ranks, std::vector< vector<long>>(config_.bankgroups, vector<long>(config_.banks_per_group, 0))),
    last_rank_refresh_(config_.ranks, 0),
    next_rank_(0)
{}

void Refresh::ClockTick() {
    clk++;
    InsertRefresh();
    return;
}

void Refresh::InsertRefresh() {
    if( clk % (config_.tREFI/config_.ranks) == 0) {
        auto addr = Address(); addr.rank_ = next_rank_;
        refresh_q_.push_back(new Request(CommandType::REFRESH, addr));
        IterateNext();
    }
    return;
}

Command Refresh::GetRefreshOrAssociatedCommand(list<Request*>::iterator req_itr) {
    auto req = *req_itr; //TODO - req_itr carlessly used (Fix)
    // Issue a single pending request 
    if( req->cmd_.cmd_type_ == CommandType::REFRESH) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto j = 0; j < config_.bankgroups; j++) {
                if(channel_state_.IsRowOpen(req->Rank(), j, k) && channel_state_.RowHitCount(req->Rank(), j, k) == 0) {
                    auto& queue = cmd_queue_.GetQueue(req->Rank(), j, k);
                    for(auto req_itr = queue.begin(); req_itr != queue.end(); req_itr++) {
                        auto req = *req_itr;
                        Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
                        if(cmd.IsReadWrite()) { //Rowhit
                            if(channel_state_.IsReady(cmd, clk)) {
                                delete(*req_itr);
                                queue.erase(req_itr);
                                return cmd;
                            }
                        }
                    }
                }
            }
        }
    }
    else if( req->cmd_.cmd_type_ == CommandType::REFRESH_BANK) {
        if(channel_state_.IsRowOpen(req->Rank(), req->Bankgroup(), req->Bank()) && channel_state_.RowHitCount(req->Rank(), req->Bankgroup(), req->Bank()) == 0) {
            auto& queue = cmd_queue_.GetQueue(req->Rank(), req->Bankgroup(), req->Bank());
            for(auto req_itr = queue.begin(); req_itr != queue.end(); req_itr++) {
                auto req = *req_itr;
                Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
                if(channel_state_.IsReady(cmd, clk)) {
                    delete(*req_itr);
                    queue.erase(req_itr);
                    return cmd;
                }
            }
        }
    }

    auto cmd = channel_state_.GetRequiredCommand(req->cmd_);
    if(channel_state_.IsReady(cmd, clk)) {
        if(req->cmd_.cmd_type_ == cmd.cmd_type_) {
            //Sought of actually issuing the refresh command
            delete(*req_itr);
            refresh_q_.erase(req_itr);
        }
        return cmd;
    }
    return Command();
}

inline void Refresh::IterateNext() {
    next_rank_ = (next_rank_ + 1) % config_.ranks;
    return;
}
