#include "channel_state.h"
#include "../ext/fmt/src/format.h"

namespace dramsim3 {
ChannelState::ChannelState(const Config& config, const Timing& timing,
                           Statistics& stats)
    :
      rank_in_self_refresh_mode_(std::vector<bool>(config.ranks, false)),
      config_(config),
      timing_(timing),
      stats_(stats),
      four_aw(config_.ranks, std::vector<uint64_t>()),
      thirty_two_aw(config_.ranks, std::vector<uint64_t>()),
      is_selfrefresh_(config_.ranks, false) {

    bank_states_.reserve(config_.ranks);
    for (auto i = 0; i < config_.ranks; i++) {
        auto rank_states = std::vector<std::vector<BankState>>();
        rank_states.reserve(config_.bankgroups);
        for (auto j = 0; j < config_.bankgroups; j++) {
            auto bg_states = std::vector<BankState>(config_.banks_per_group,
                                                    BankState(stats));
            rank_states.push_back(bg_states);
        }
        bank_states_.push_back(rank_states);
    }

}

bool ChannelState::IsAllBankIdleInRank(int rank) const {
    for (int j = 0; j < config_.bankgroups; j++) {
        for (int k = 0; k < config_.banks_per_group; k++) {
            if (bank_states_[rank][j][k].IsRowOpen()) {
                return false;
            }
        }
    }
    return true;
}

bool ChannelState::IsRefreshWaiting(int rank, int bankgroup, int bank) const {
    return bank_states_[rank][bankgroup][bank].IsRefreshWaiting();
}

void ChannelState::BankNeedRefresh(int rank, int bankgroup, int bank, bool need) {
    bank_states_[rank][bankgroup][bank].NeedRefresh(need);
    return;
}

void ChannelState::RankNeedRefresh(int rank, bool need) {
    for (int j = 0; j < config_.bankgroups; j++) {
        for (int k = 0; k < config_.banks_per_group; k++) {
            bank_states_[rank][j][k].NeedRefresh(need);
        }
    }
    return;
}

Command ChannelState::GetRequiredCommand(const Command& cmd) const {
    CommandType cmd_type = cmd.cmd_type;
    Address addr = Address(cmd.addr);
    int cmd_id = cmd.id;
    bool need_precharge = false;
    switch (cmd.cmd_type) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            cmd_type = bank_states_[cmd.Rank()][cmd.Bankgroup()][cmd.Bank()]
                               .GetRequiredCommandType(cmd);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            // Static fixed order to check banks
            for (auto j = 0; j < config_.bankgroups; j++) {
                for (auto k = 0; k < config_.banks_per_group; k++) {
                    CommandType tmp_type =
                        bank_states_[cmd.Rank()][j][k].GetRequiredCommandType(
                            cmd);
                    if (tmp_type != cmd.cmd_type) {  // precharge
                        need_precharge = true;
                        cmd_type = tmp_type;
                        addr.bankgroup = j;
                        addr.bank = k;
                        addr.row = bank_states_[cmd.Rank()][j][k].OpenRow();
                        cmd_id = -1;
                        break;
                    }
                }
                if (need_precharge){
                    break;
                }
            }
            break;
        default:
            std::cerr << cmd << std::endl;
            AbruptExit(__FILE__, __LINE__);
            break;
    }
    return Command(cmd_type, addr, cmd_id); 
}

bool ChannelState::IsReady(const Command& cmd, uint64_t clk) const {
    switch (cmd.cmd_type) {
        case CommandType::ACTIVATE:
            if (!ActivationWindowOk(cmd.Rank(), clk))
                return false;  // TODO - Bad coding. Case statement is not
                               // supposed to be used like this
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            return bank_states_[cmd.Rank()][cmd.Bankgroup()][cmd.Bank()]
                .IsReady(cmd.cmd_type, clk);
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT: {
            bool is_ready = true;
            for (auto j = 0; j < config_.bankgroups; j++) {
                for (auto k = 0; k < config_.banks_per_group; k++) {
                    is_ready &= bank_states_[cmd.Rank()][j][k].IsReady(
                        cmd.cmd_type, clk);
                    // if(!is_ready) return false //Early return for simulator
                    // performance?
                }
            }
            return is_ready;
        }
        default:
            AbruptExit(__FILE__, __LINE__);
            return true;
    }
}

void ChannelState::UpdateState(const Command& cmd) {
    switch (cmd.cmd_type) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            bank_states_[cmd.Rank()][cmd.Bankgroup()][cmd.Bank()].UpdateState(
                cmd);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            is_selfrefresh_[cmd.Rank()] =
                (cmd.cmd_type == CommandType::SELF_REFRESH_ENTER ? true
                                                                 : false);
            for (auto j = 0; j < config_.bankgroups; j++) {
                for (auto k = 0; k < config_.banks_per_group; k++) {
                    bank_states_[cmd.Rank()][j][k].UpdateState(cmd);
                }
            }
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    switch (cmd.cmd_type) {
        case CommandType::SELF_REFRESH_ENTER:
            rank_in_self_refresh_mode_[cmd.Rank()] = true;
            break;
        case CommandType::SELF_REFRESH_EXIT:
            rank_in_self_refresh_mode_[cmd.Rank()] = false;
            break;
        default:
            break;
    }
    return;
}

void ChannelState::UpdateTiming(const Command& cmd, uint64_t clk) {
    switch (cmd.cmd_type) {
        case CommandType::ACTIVATE:
            UpdateActivationTimes(
                cmd.Rank(), clk);  // TODO - Bad coding. Note : no break here
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            // TODO - simulator speed? - Speciazlize which of the below
            // functions to call depending on the command type  Same Bank
            UpdateSameBankTiming(
                cmd.addr, timing_.same_bank[static_cast<int>(cmd.cmd_type)],
                clk);

            // Same Bankgroup other banks
            UpdateOtherBanksSameBankgroupTiming(
                cmd.addr,
                timing_
                    .other_banks_same_bankgroup[static_cast<int>(cmd.cmd_type)],
                clk);

            // Other bankgroups
            UpdateOtherBankgroupsSameRankTiming(
                cmd.addr,
                timing_
                    .other_bankgroups_same_rank[static_cast<int>(cmd.cmd_type)],
                clk);

            // Other ranks
            UpdateOtherRanksTiming(
                cmd.addr, timing_.other_ranks[static_cast<int>(cmd.cmd_type)],
                clk);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            UpdateSameRankTiming(
                cmd.addr, timing_.same_rank[static_cast<int>(cmd.cmd_type)],
                clk);
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    return;
}

void ChannelState::UpdateSameBankTiming(
    const Address& addr,
    const std::vector<std::pair<CommandType, int>>& cmd_timing_list,
    uint64_t clk) {
    for (auto cmd_timing : cmd_timing_list) {
        bank_states_[addr.rank][addr.bankgroup][addr.bank].UpdateTiming(
            cmd_timing.first, clk + cmd_timing.second);
    }
    return;
}

void ChannelState::UpdateOtherBanksSameBankgroupTiming(
    const Address& addr,
    const std::vector<std::pair<CommandType, int>>& cmd_timing_list,
    uint64_t clk) {
    for (auto k = 0; k < config_.banks_per_group; k++) {
        if (k != addr.bank) {
            for (auto cmd_timing : cmd_timing_list) {
                bank_states_[addr.rank][addr.bankgroup][k].UpdateTiming(
                    cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void ChannelState::UpdateOtherBankgroupsSameRankTiming(
    const Address& addr,
    const std::vector<std::pair<CommandType, int>>& cmd_timing_list,
    uint64_t clk) {
    for (auto j = 0; j < config_.bankgroups; j++) {
        if (j != addr.bankgroup) {
            for (auto k = 0; k < config_.banks_per_group; k++) {
                for (auto cmd_timing : cmd_timing_list) {
                    bank_states_[addr.rank][j][k].UpdateTiming(
                        cmd_timing.first, clk + cmd_timing.second);
                }
            }
        }
    }
    return;
}

void ChannelState::UpdateOtherRanksTiming(
    const Address& addr,
    const std::vector<std::pair<CommandType, int>>& cmd_timing_list,
    uint64_t clk) {
    for (auto i = 0; i < config_.ranks; i++) {
        if (i != addr.rank) {
            for (auto j = 0; j < config_.bankgroups; j++) {
                for (auto k = 0; k < config_.banks_per_group; k++) {
                    for (auto cmd_timing : cmd_timing_list) {
                        bank_states_[i][j][k].UpdateTiming(
                            cmd_timing.first, clk + cmd_timing.second);
                    }
                }
            }
        }
    }
    return;
}

void ChannelState::UpdateSameRankTiming(
    const Address& addr,
    const std::vector<std::pair<CommandType, int>>& cmd_timing_list,
    uint64_t clk) {
    for (auto j = 0; j < config_.bankgroups; j++) {
        for (auto k = 0; k < config_.banks_per_group; k++) {
            for (auto cmd_timing : cmd_timing_list) {
                bank_states_[addr.rank][j][k].UpdateTiming(
                    cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void ChannelState::UpdateTimingAndStates(const Command& cmd, uint64_t clk) {
    UpdateState(cmd);
    UpdateTiming(cmd, clk);
    UpdateCommandIssueStats(cmd);
    return;
}


bool ChannelState::ActivationWindowOk(int rank, uint64_t curr_time) const {
    bool tfaw_ok = IsFAWReady(rank, curr_time);
    if (config_.IsGDDR()) {
        if (!tfaw_ok)
            return false;
        else
            return Is32AWReady(rank, curr_time);
    }
    return tfaw_ok;
}

void ChannelState::UpdateActivationTimes(int rank, uint64_t curr_time) {
    if (!four_aw[rank].empty() && curr_time >= four_aw[rank][0]) {
        four_aw[rank].erase(four_aw[rank].begin());
    }
    four_aw[rank].push_back(curr_time + config_.tFAW);
    if (config_.IsGDDR()) {
        if (!thirty_two_aw[rank].empty() &&
            curr_time >= thirty_two_aw[rank][0]) {
            thirty_two_aw[rank].erase(thirty_two_aw[rank].begin());
        }
        thirty_two_aw[rank].push_back(curr_time + config_.t32AW);
    }
    return;
}

bool ChannelState::IsFAWReady(int rank, uint64_t curr_time) const {
    if (!four_aw[rank].empty()) {
        if (curr_time < four_aw[rank][0] && four_aw[rank].size() >= 4) {
            return false;
        }
    }
    return true;
}

bool ChannelState::Is32AWReady(int rank, uint64_t curr_time) const {
    if (!thirty_two_aw[rank].empty()) {
        if (curr_time < thirty_two_aw[rank][0] &&
            thirty_two_aw[rank].size() >= 32) {
            return false;
        }
    }
    return true;
}

void ChannelState::UpdateCommandIssueStats(const Command& cmd) const {
    switch (cmd.cmd_type) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
            stats_.numb_read_cmds_issued++;
            break;
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
            stats_.numb_write_cmds_issued++;
            break;
        case CommandType::ACTIVATE:
            stats_.numb_activate_cmds_issued++;
            break;
        case CommandType::PRECHARGE:
            stats_.numb_precharge_cmds_issued++;
            break;
        case CommandType::REFRESH:
            stats_.numb_refresh_cmds_issued++;
            break;
        case CommandType::REFRESH_BANK:
            stats_.numb_refresh_bank_cmds_issued++;
            break;
        case CommandType::SELF_REFRESH_ENTER:
            stats_.numb_self_refresh_enter_cmds_issued++;
            break;
        case CommandType::SELF_REFRESH_EXIT:
            stats_.numb_self_refresh_exit_cmds_issued++;
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    return;
}

}  // namespace dramsim3
