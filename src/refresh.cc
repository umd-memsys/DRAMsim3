#include "refresh.h"

using namespace std;
using namespace dramcore;

Refresh::Refresh(const uint32_t channel_id, const Config &config, const ChannelState &channel_state, CommandQueue &cmd_queue, Statistics &stats) :
    clk_(0),
    channel_id_(channel_id),
    config_(config),
    channel_state_(channel_state),
    cmd_queue_(cmd_queue),
    stats_(stats),
    last_bank_refresh_(config_.ranks, std::vector< vector<uint64_t>>(config_.bankgroups, vector<uint64_t>(config_.banks_per_group, 0))),
    last_rank_refresh_(config_.ranks, 0),
    next_rank_(0),
    next_bankgroup_(0),
    next_bank_(0)
{
    if(config.refresh_strategy == "RANK_LEVEL_SIMULTANEOUS") {
        refresh_strategy_ = RefreshStrategy::RANK_LEVEL_SIMULTANEOUS;
    }
    else if(config.refresh_strategy == "RANK_LEVEL_STAGGERED") {
        refresh_strategy_ = RefreshStrategy::RANK_LEVEL_STAGGERED;
    }
    else if(config.refresh_strategy == "BANK_LEVEL_SIMULTANEOUS") {
        refresh_strategy_ = RefreshStrategy ::BANK_LEVEL_SIMULTANEOUS;
    }
    else if(config.refresh_strategy == "BANK_LEVEL_STAGGERED") {
        refresh_strategy_ = RefreshStrategy::BANK_LEVEL_STAGGERED;
    }
    else {
        AbruptExit(__FILE__, __LINE__);
    }
}

void Refresh::ClockTick() {
    clk_++;
    InsertRefresh();
    return;
}

void Refresh::InsertRefresh() {
    switch(refresh_strategy_) {
        case RefreshStrategy::RANK_LEVEL_SIMULTANEOUS: // //Simultaneous all rank refresh
            if (clk_ % config_.tREFI == 0) {
                for (auto i = 0; i < config_.ranks; i++) {
                    if(!channel_state_.rank_in_self_refresh_mode_[i]) {
                        auto addr = Address();
                        addr.channel_ = channel_id_;
                        addr.rank_ = i;
                        refresh_q_.push_back(new Request(CommandType::REFRESH, addr));
                    }
                }
            }
            return;
        case RefreshStrategy::RANK_LEVEL_STAGGERED: //Staggered all rank refresh
            if (clk_ % (config_.tREFI / config_.ranks) == 0) {
                if(!channel_state_.rank_in_self_refresh_mode_[next_rank_]) {
                    auto addr = Address();
                    addr.channel_ = channel_id_;
                    addr.rank_ = next_rank_;
                    refresh_q_.push_back(new Request(CommandType::REFRESH, addr));
                }
                IterateNext();
            }
            return;
        case RefreshStrategy::BANK_LEVEL_SIMULTANEOUS: //rank level staggered but bank level simultaneous per bank refresh
            if (clk_ % (config_.tREFI / config_.ranks) == 0) {
                if(!channel_state_.rank_in_self_refresh_mode_[next_rank_]) {
                    for (auto k = 0; k < config_.banks_per_group; k++) {
                        for (auto j = 0; j < config_.bankgroups; j++) {
                            auto addr = Address();
                            addr.channel_ = channel_id_;
                            addr.rank_ = next_rank_;
                            addr.bankgroup_ = j;
                            addr.bank_ = k;
                            refresh_q_.push_back(new Request(CommandType::REFRESH_BANK, addr));
                        }
                    }
                }
                IterateNext();
            }
            return;
        case RefreshStrategy::BANK_LEVEL_STAGGERED: //Fully staggered per bank refresh
            // if (clk_ % (config_.tREFI / config_.ranks) == 0) { //TODO - tREFI needs to a multiple of numb_ranks
            if (clk_ % config_.tREFIb == 0) {
                if(!channel_state_.rank_in_self_refresh_mode_[next_rank_]) {
                    auto addr = Address();
                    addr.channel_ = channel_id_;
                    addr.rank_ = next_rank_;
                    addr.bankgroup_ = next_bankgroup_;
                    addr.bank_ = next_bank_;
                    refresh_q_.push_back(new Request(CommandType::REFRESH_BANK, addr));
                }
                IterateNext();
            }
            return;
        default:
            AbruptExit(__FILE__, __LINE__);
            return;
    }
}

Command Refresh::GetRefreshOrAssociatedCommand(list<Request*>::iterator refresh_itr) {
    auto refresh_req = *refresh_itr;
    //TODO - Strict round robin search of queues?
    if( refresh_req->cmd_.cmd_type_ == CommandType::REFRESH) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto j = 0; j < config_.bankgroups; j++) {
                if(ReadWritesToFinish(refresh_req->Rank(), j, k)) {
                    return GetReadWritesToOpenRow(refresh_req->Rank(), j, k);
                }
            }
        }
    }
    else if( refresh_req->cmd_.cmd_type_ == CommandType::REFRESH_BANK) {
        if(ReadWritesToFinish(refresh_req->Rank(), refresh_req->Bankgroup(), refresh_req->Bank())) {
            return GetReadWritesToOpenRow(refresh_req->Rank(), refresh_req->Bankgroup(), refresh_req->Bank());
        }
    }

    auto cmd = channel_state_.GetRequiredCommand(refresh_req->cmd_);
    if(channel_state_.IsReady(cmd, clk_)) {
        if(cmd.IsRefresh()) {
            //Sought of actually issuing the refresh command
            delete(*refresh_itr);
            refresh_q_.erase(refresh_itr);
        }
        return cmd;
    }
    return Command();
}


bool Refresh::ReadWritesToFinish(int rank, int bankgroup, int bank) {
    // Issue a single pending request
    return channel_state_.IsRowOpen(rank, bankgroup, bank) && channel_state_.RowHitCount(rank, bankgroup, bank) == 0;
}

Command Refresh::GetReadWritesToOpenRow(uint32_t rank, uint32_t bankgroup, uint32_t bank) {
    auto& queue = cmd_queue_.GetQueue(rank, bankgroup, bank);
    for(auto req_itr = queue.begin(); req_itr != queue.end(); req_itr++) {
        auto req = *req_itr;
        //If the req belongs to the same bank which is being checked if it is okay for refresh
        //Necessary for PER_RANK queues
        if(req->Rank() == rank && req->Bankgroup() == bankgroup && req->Bank() == bank) {
            Command cmd = channel_state_.GetRequiredCommand(req->cmd_);
            if (channel_state_.IsReady(cmd, clk_)) {
                cmd_queue_.IssueRequest(queue, req_itr);
                stats_.numb_rw_rowhits_pending_refresh++;
                return cmd;
            }
        }
    }
    return Command();
}


inline void Refresh::IterateNext() {
    switch(refresh_strategy_) {
        case RefreshStrategy::RANK_LEVEL_STAGGERED:
        case RefreshStrategy::BANK_LEVEL_SIMULTANEOUS:
            next_rank_ = (next_rank_ + 1) % config_.ranks;
            return;
        case RefreshStrategy::BANK_LEVEL_STAGGERED:
            // Note - the order issuing bank refresh commands is static and non-configurable as per JEDEC standard
            next_bankgroup_ = (next_bankgroup_ + 1) % config_.bankgroups;
            if (next_bankgroup_ == 0) {
                next_bank_ = (next_bank_ + 1) % config_.banks_per_group;
                if (next_bank_ == 0) {
                    next_rank_ = (next_rank_ + 1) % config_.ranks;
                }
            }
            return;
        default:
            AbruptExit(__FILE__, __LINE__);
            return;
    }
}
