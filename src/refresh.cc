#include "refresh.h"

namespace dramsim3 {
Refresh::Refresh(const Config &config,
                 const ChannelState &channel_state, CommandQueue &cmd_queue,
                 Statistics &stats)
    : clk_(0),
      config_(config),
      channel_state_(channel_state),
      cmd_queue_(cmd_queue),
      stats_(stats),
      next_rank_(0),
      next_bankgroup_(0),
      next_bank_(0) {

    for (auto i = 0; i < config_.ranks; i++) {
        auto rank_ref_status = std::vector<std::vector<bool>> ();
        rank_ref_status.reserve(config_.bankgroups);
        for (auto j = 0; j < config_.bankgroups; j++) {
            auto bg_ref_status = std::vector<bool>(config_.banks_per_group,
                                                   false);
            rank_ref_status.push_back(bg_ref_status);
        }
        refresh_waiting_.push_back(rank_ref_status);
    }

    if (config.refresh_strategy == "RANK_LEVEL_SIMULTANEOUS") {
        refresh_strategy_ = RefreshStrategy::RANK_LEVEL_SIMULTANEOUS;
        refresh_interval_ = config_.tREFI;
    } else if (config.refresh_strategy == "RANK_LEVEL_STAGGERED") {
        refresh_strategy_ = RefreshStrategy::RANK_LEVEL_STAGGERED;
        refresh_interval_ = config_.tREFI / config_.ranks;
    } else if (config.refresh_strategy == "BANK_LEVEL_STAGGERED") {
        refresh_strategy_ = RefreshStrategy::BANK_LEVEL_STAGGERED;
        refresh_interval_ = config_.tREFIb;
    } else {
        AbruptExit(__FILE__, __LINE__);
    }
}

void Refresh::ClockTick() {
    if (clk_ % refresh_interval_ == 0 && clk_ > 0) {
        InsertRefresh();
    }
    clk_++;
    return;
}

bool Refresh::IsRefWaiting() {
    return !refresh_q_.empty();
}

void Refresh::RefreshIssued(const Command& cmd) {
    UpdateRefWaiting(cmd, false);
    return;
}

void Refresh::UpdateRefWaiting(const Command& cmd, bool status) {
    if (cmd.cmd_type == CommandType::REFRESH) {
        for (auto j = 0; j < config_.bankgroups; j++) {
            for (auto k = 0; k < config_.banks_per_group; k++) {
                refresh_waiting_[cmd.Rank()][j][k] = status;
            }
        }
    } else if (cmd.cmd_type == CommandType::REFRESH_BANK) {
        for (auto k = 0; k < config_.banks_per_group; k++) {
            refresh_waiting_[cmd.Rank()][cmd.Bankgroup()][k] = status;
        }
    } else {
        AbruptExit(__FILE__, __LINE__);
    }
    return;
}

void Refresh::InsertRefresh() {
    Address addr = Address();
    CommandType cmd_type = CommandType::REFRESH;
    switch (refresh_strategy_) {
        case RefreshStrategy::RANK_LEVEL_SIMULTANEOUS:  // Simultaneous all rank
                                                        // refresh
            for (auto i = 0; i < config_.ranks; i++) {
                if (!channel_state_.rank_in_self_refresh_mode_[i]) {
                    addr.rank = i;
                }
            }
            break;
        case RefreshStrategy::RANK_LEVEL_STAGGERED:  // Staggered all rank
                                                     // refresh
            if (!channel_state_.rank_in_self_refresh_mode_[next_rank_]) {
                addr.rank = next_rank_;
            }
            IterateNext();
            break;
        case RefreshStrategy::BANK_LEVEL_STAGGERED:  // Fully staggered per bank
                                                     // refresh
            if (!channel_state_.rank_in_self_refresh_mode_[next_rank_]) {
                addr.rank = next_rank_;
                addr.bankgroup = next_bankgroup_;
                addr.bank = next_bank_;
                cmd_type = CommandType::REFRESH_BANK;
            }
            IterateNext();
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
            break;
    }
    Command cmd = Command(cmd_type, addr, -1);
    UpdateRefWaiting(cmd, true);
    refresh_q_.push_back(cmd);
    return;
}

Command Refresh::GetRefreshOrAssociatedCommand() {
    auto refresh_itr = refresh_q_.begin();
    auto refresh_req = *refresh_itr;
    // auto refresh_req = *refresh_itr;
    // TODO - Strict round robin search of queues?
    if (refresh_req.cmd_type == CommandType::REFRESH) {
        for (auto k = 0; k < config_.banks_per_group; k++) {
            for (auto j = 0; j < config_.bankgroups; j++) {
                if (ReadWritesToFinish(refresh_req.Rank(), j, k)) {
                    return GetReadWritesToOpenRow(refresh_req.Rank(), j, k);
                }
            }
        }
    } else if (refresh_req.cmd_type == CommandType::REFRESH_BANK) {
        if (ReadWritesToFinish(refresh_req.Rank(), refresh_req.Bankgroup(),
                               refresh_req.Bank())) {
            return GetReadWritesToOpenRow(refresh_req.Rank(),
                                          refresh_req.Bankgroup(),
                                          refresh_req.Bank());
        }
    }

    auto cmd = channel_state_.GetRequiredCommand(refresh_req);
    if (channel_state_.IsReady(cmd, clk_)) {
        if (cmd.IsRefresh()) {
            // Sought of actually issuing the refresh command
            refresh_q_.erase(refresh_itr);
        }
        return cmd;
    }
    return Command();
}

bool Refresh::ReadWritesToFinish(int rank, int bankgroup, int bank) {
    // Issue a single pending request
    return channel_state_.IsRowOpen(rank, bankgroup, bank) &&
           channel_state_.RowHitCount(rank, bankgroup, bank) == 0;
}

Command Refresh::GetReadWritesToOpenRow(int rank, int bankgroup, int bank) {
    auto &queue = cmd_queue_.GetQueue(rank, bankgroup, bank);
    for (auto cmd_it = queue.begin(); cmd_it != queue.end(); cmd_it++) {
        // If the cmd belongs to the same bank which is being checked if it is
        // okay for refresh  Necessary for PER_RANK queues
        if (cmd_it->Rank() == rank && cmd_it->Bankgroup() == bankgroup &&
            cmd_it->Bank() == bank) {
            Command cmd = channel_state_.GetRequiredCommand(*cmd_it);
            if (channel_state_.IsReady(cmd, clk_)) {
                stats_.numb_rw_rowhits_pending_refresh++;
                // TODO Dumbest shit I have to ever write
                if (cmd.IsReadWrite()) {
                    queue.erase(cmd_it);
                }
                return cmd;
            }
        }
    }
    return Command();
}

inline void Refresh::IterateNext() {
    switch (refresh_strategy_) {
        case RefreshStrategy::RANK_LEVEL_STAGGERED:
            next_rank_ = (next_rank_ + 1) % config_.ranks;
            return;
        case RefreshStrategy::BANK_LEVEL_STAGGERED:
            // Note - the order issuing bank refresh commands is static and
            // non-configurable as per JEDEC standard
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

}  // namespace dramsim3
