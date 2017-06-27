#include "channel_state.h"
#include "../ext/fmt/src/format.h"

using namespace std;
using namespace dramcore;

ChannelState::ChannelState(const Config &config, const Timing &timing, Statistics &stats) :
    need_to_update_refresh_waiting_status_(true),
    rank_in_self_refresh_mode_(std::vector<bool>(config.ranks, false)),
    config_(config),
    timing_(timing),
    stats_(stats),
    bank_states_(config_.ranks, std::vector< std::vector<BankState*> >(config_.bankgroups, std::vector<BankState*>(config_.banks_per_group, NULL) ) ),
    four_aw(config_.ranks, std::vector<uint64_t>()),
    thirty_two_aw(config_.ranks, std::vector<uint64_t>()),
    is_selfrefresh_(config_.ranks, false)
{
    for(auto i = 0; i < config_.ranks; i++) {
        for(auto j = 0; j < config_.bankgroups; j++) {
            for(auto k = 0; k < config_.banks_per_group; k++) {
                bank_states_[i][j][k] = new BankState(stats);
            }
        }
    }
    if (!config.validation_output_file.empty()) {
        cout << "Validation Command Trace write to "<< config.validation_output_file << endl;
        val_output_enable = true;
        val_output_.open(config.validation_output_file, std::ofstream::out);
    }
}

int ChannelState::ActiveBanksInRank(int rank) const {
    int count = 0;
    for (unsigned j = 0; j < config_.bankgroups; j++) {
        for (unsigned k = 0; k < config_.banks_per_group; k++) {
            if (bank_states_[rank][j][k]->IsRowOpen()) {
                count++;
            }
        }
    }
    return count;
}



Command ChannelState::GetRequiredCommand(const Command& cmd) const {
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            return Command(bank_states_[cmd.Rank()][cmd.Bankgroup()][cmd.Bank()]->GetRequiredCommandType(cmd), cmd.addr_);
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            //Static fixed order to check banks
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    CommandType required_cmd_type = bank_states_[cmd.Rank()][j][k]->GetRequiredCommandType(cmd);
                    if( required_cmd_type != cmd.cmd_type_) {
                        auto addr = Address(cmd.addr_);
                        addr.bankgroup_ = j;
                        addr.bank_ = k;
                        return Command(required_cmd_type, addr);
                    }
                }
            }
            return cmd;
        default:
            AbruptExit(__FILE__, __LINE__);
            return Command();
    }
}

bool ChannelState::IsReady(const Command& cmd, uint64_t clk) const {
    switch(cmd.cmd_type_) {
        case CommandType::ACTIVATE:
            if(!ActivationWindowOk(cmd.Rank(), clk))
                return false; //TODO - Bad coding. Case statement is not supposed to be used like this
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            return bank_states_[cmd.Rank()][cmd.Bankgroup()][cmd.Bank()]->IsReady(cmd.cmd_type_, clk);
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT: {
            bool is_ready = true;
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    is_ready &= bank_states_[cmd.Rank()][j][k]->IsReady(cmd.cmd_type_, clk);
                    //if(!is_ready) return false //Early return for simulator performance?
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
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            bank_states_[cmd.Rank()][cmd.Bankgroup()][cmd.Bank()]->UpdateState(cmd);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            is_selfrefresh_[cmd.Rank()] = (cmd.cmd_type_ == CommandType::SELF_REFRESH_ENTER ? true : false);
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    bank_states_[cmd.Rank()][j][k]->UpdateState(cmd);
                }
            }
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    switch(cmd.cmd_type_) {
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
    switch(cmd.cmd_type_) {
        case CommandType::ACTIVATE:
            UpdateActivationTimes(cmd.Rank(), clk); //TODO - Bad coding. Note : no break here
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            //TODO - simulator speed? - Speciazlize which of the below functions to call depending on the command type
            //Same Bank
            UpdateSameBankTiming(cmd.addr_, timing_.same_bank[static_cast<int>(cmd.cmd_type_)], clk);

            //Same Bankgroup other banks
            UpdateOtherBanksSameBankgroupTiming(cmd.addr_, timing_.other_banks_same_bankgroup[static_cast<int>(cmd.cmd_type_)], clk);

            //Other bankgroups
            UpdateOtherBankgroupsSameRankTiming(cmd.addr_, timing_.other_bankgroups_same_rank[static_cast<int>(cmd.cmd_type_)], clk);

            //Other ranks
            UpdateOtherRanksTiming(cmd.addr_, timing_.other_ranks[static_cast<int>(cmd.cmd_type_)], clk);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            UpdateSameRankTiming(cmd.addr_, timing_.same_rank[static_cast<int>(cmd.cmd_type_)], clk);
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }
    return ;
}

void ChannelState::UpdateSameBankTiming(const Address& addr, const std::list< std::pair<CommandType, uint32_t> >& cmd_timing_list, uint64_t clk) {
    for(auto cmd_timing : cmd_timing_list ) {
        bank_states_[addr.rank_][addr.bankgroup_][addr.bank_]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
    }
    return;
}

void ChannelState::UpdateOtherBanksSameBankgroupTiming(const Address& addr, const std::list< std::pair<CommandType, uint32_t> >& cmd_timing_list, uint64_t clk) {
    for(auto k = 0; k < config_.banks_per_group; k++) {
        if( k != addr.bank_) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[addr.rank_][addr.bankgroup_][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void ChannelState::UpdateOtherBankgroupsSameRankTiming(const Address& addr, const std::list< std::pair<CommandType, uint32_t> >& cmd_timing_list, uint64_t clk) {
    for(auto j = 0; j < config_.bankgroups; j++) {
        if(j != addr.bankgroup_) {
            for(auto k = 0; k < config_.banks_per_group; k++) {
                for(auto cmd_timing : cmd_timing_list ) {
                    bank_states_[addr.rank_][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
                }
            }
        }
    }
    return;
}

void ChannelState::UpdateOtherRanksTiming(const Address& addr, const std::list< std::pair<CommandType, uint32_t> >& cmd_timing_list, uint64_t clk) {
    for(auto i = 0; i < config_.ranks; i++) {
        if(i != addr.rank_) {
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    for(auto cmd_timing : cmd_timing_list ) {
                        bank_states_[i][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
                    }
                }
            }
        }
    }
    return;
}

void ChannelState::UpdateSameRankTiming(const Address& addr, const std::list< std::pair<CommandType, uint32_t> >& cmd_timing_list, uint64_t clk) {
    for(auto j = 0; j < config_.bankgroups; j++) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[addr.rank_][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void ChannelState::IssueCommand(const Command& cmd, uint64_t clk) {
    #ifdef DEBUG_OUTPUT
        cout << left << setw(8) << clk << " " << cmd << endl;
    #endif  //DEBUG_OUTPUT
    if (val_output_enable) {
        val_output_ << left << setw(8) << clk << " " << cmd <<std::endl;
    }
    UpdateState(cmd);
    UpdateTiming(cmd, clk);
    UpdateCommandIssueStats(cmd);
    return;
}

void ChannelState::UpdateRefreshWaitingStatus(const Command& cmd, bool status) {
    if(cmd.cmd_type_ == CommandType::REFRESH) {
        for( auto j = 0; j < config_.bankgroups; j++) {
            for( auto k = 0; k < config_.banks_per_group; k++) {
                bank_states_[cmd.Rank()][j][k]->UpdateRefreshWaitingStatus(status);
            }
        }
    }
    else if(cmd.cmd_type_ == CommandType::REFRESH_BANK) {
        for( auto k = 0; k < config_.banks_per_group; k++) {
            bank_states_[cmd.Rank()][cmd.Bankgroup()][k]->UpdateRefreshWaitingStatus(status);
        }
    }
    else {
        AbruptExit(__FILE__, __LINE__);
    }
    return;
}

bool ChannelState::ActivationWindowOk(int rank, uint64_t curr_time) const {
    bool tfaw_ok = IsFAWReady(rank, curr_time);
    if(config_.IsGDDR()) {
        if(!tfaw_ok)
            return false;
        else
            return Is32AWReady(rank, curr_time);
    }
    return tfaw_ok;
}

void ChannelState::UpdateActivationTimes(int rank, uint64_t curr_time) {
    if(!four_aw[rank].empty() && curr_time >= four_aw[rank][0]) {
        four_aw[rank].erase(four_aw[rank].begin());
    }
    four_aw[rank].push_back(curr_time + config_.tFAW);
    if(config_.IsGDDR()) {
        if(!thirty_two_aw[rank].empty() && curr_time >= thirty_two_aw[rank][0]) {
            thirty_two_aw[rank].erase(thirty_two_aw[rank].begin());
        }
        thirty_two_aw[rank].push_back(curr_time + config_.t32AW);
    }
    return;
}

bool ChannelState::IsFAWReady(int rank, uint64_t curr_time) const {
    if(!four_aw[rank].empty()) {
        if( curr_time < four_aw[rank][0] && four_aw[rank].size() >= 4) {
            return false;
        } 
    } 
    return true;
}

bool ChannelState::Is32AWReady(int rank, uint64_t curr_time) const {
    if(!thirty_two_aw[rank].empty()) {
        if ( curr_time < thirty_two_aw[rank][0] && thirty_two_aw[rank].size() >= 32) {
            return false;
        }
    } 
    return true;
}

void ChannelState::UpdateCommandIssueStats(const Command& cmd) const {
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
            stats_.numb_read_cmds_issued++;
            stats_.read_energy++;
            break;
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
            stats_.numb_write_cmds_issued++;
            stats_.write_energy++;
            break;
        case CommandType::ACTIVATE:
            stats_.numb_activate_cmds_issued++;
            stats_.act_energy++;
            break;
        case CommandType::PRECHARGE:
            stats_.numb_precharge_cmds_issued++;
            break;
        case CommandType::REFRESH:
            stats_.numb_refresh_cmds_issued++;
            stats_.ref_energy++;
            break;
        case CommandType::REFRESH_BANK:
            stats_.numb_refresh_bank_cmds_issued++;
            stats_.refb_energy++;
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

