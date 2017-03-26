#include "channel_state.h"

using namespace std;

ChannelState::ChannelState(const Config& config, const Timing& timing) :
    config_(config),
    timing_(timing),
    bank_states_(config_.ranks, vector< vector<BankState*> >(config_.bankgroups, vector<BankState*>(config_.banks_per_group, NULL) ) ),
    activation_times_(config_.ranks, list<long>())
{
    for(auto i = 0; i < config_.ranks; i++) {
        for(auto j = 0; j < config_.bankgroups; j++) {
            for(auto k = 0; k < config_.banks_per_group; k++) {
                bank_states_[i][j][k] = new BankState(i, j, k);
            }
        }
    }
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
            return Command(cmd, bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->GetRequiredCommandType(cmd)); //TODO - Modify Command constructor to (addr, cmd_type)
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            //Static fixed order to check banks
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    CommandType required_cmd_type = bank_states_[cmd.rank_][j][k]->GetRequiredCommandType(cmd);
                    if( required_cmd_type != cmd.cmd_type_)
                        return Command(required_cmd_type, -1, cmd.rank_, j, k, -1); //Terrible coding. 
                }
            }
            return cmd;
            break;
        default:
            exit(-1);
    }
}

bool ChannelState::IsReady(const Command& cmd, long clk) const {
    if(cmd.cmd_type_ != CommandType::REFRESH && cmd.cmd_type_ != CommandType::PRECHARGE && IsRefreshWaiting(cmd.rank_, cmd.bankgroup_, cmd.bank_))
        return false;
    switch(cmd.cmd_type_) {
        case CommandType::ACTIVATE:
            if(ActivationConstraint(cmd.rank_, clk))
                return false; //TODO - Bad coding. Case statement is not supposed to be used like this
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            return bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->IsReady(cmd.cmd_type_, clk);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT: {
            bool is_ready = true;
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    is_ready &= bank_states_[cmd.rank_][j][k]->IsReady(cmd.cmd_type_, clk);
                    //if(!is_ready) return false //Early return for simulator performance?
                }
            }
            return is_ready;
            break;
        }
        default:
            exit(-1);
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
            bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->UpdateState(cmd);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    bank_states_[cmd.rank_][j][k]->UpdateState(cmd);
                }
            }
            break;
        default:
            exit(-1);
    }
}

void ChannelState::UpdateTiming(const Command& cmd, long clk) {
    switch(cmd.cmd_type_) {
        case CommandType::ACTIVATE:
            UpdateActivationTimes(cmd.rank_, clk); //TODO - Bad coding. Note : no break here
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            //TODO - simulator speed? - Speciazlize which of the below functions to call depending on the command type
            //Same Bank
            UpdateSameBankTiming(cmd.rank_, cmd.bankgroup_, cmd.bank_, timing_.same_bank[static_cast<int>(cmd.cmd_type_)], clk); //TODO - Just pass in addr, cmd_type, clk

            //Same Bankgroup other banks
            UpdateOtherBanksSameBankgroupTiming(cmd.rank_, cmd.bankgroup_, cmd.bank_, timing_.other_banks_same_bankgroup[static_cast<int>(cmd.cmd_type_)], clk); //TODO - same as above

            //Other bankgroups
            UpdateOtherBankgroupsSameRankTiming(cmd.rank_, cmd.bankgroup_, timing_.other_bankgroups_same_rank[static_cast<int>(cmd.cmd_type_)], clk); //TODO - same as above

            //Other ranks
            UpdateOtherRanksTiming(cmd.rank_, timing_.other_ranks[static_cast<int>(cmd.cmd_type_)], clk); //TODO - same as above
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            UpdateSameRankTiming(cmd.rank_, timing_.same_rank[static_cast<int>(cmd.cmd_type_)], clk);
            break;
        default:
            exit(-1);
    }
    return ;
}

void ChannelState::UpdateSameBankTiming(int rank, int bankgroup, int bank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto cmd_timing : cmd_timing_list ) {
        bank_states_[rank][bankgroup][bank]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
    }
    return;
}

void ChannelState::UpdateOtherBanksSameBankgroupTiming(int rank, int bankgroup, int bank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto k = 0; k < config_.banks_per_group; k++) {
        if( k != bank) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[rank][bankgroup][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void ChannelState::UpdateOtherBankgroupsSameRankTiming(int rank, int bankgroup, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto j = 0; j < config_.bankgroups; j++) {
        if(j != bankgroup) {
            for(auto k = 0; k < config_.banks_per_group; k++) {
                for(auto cmd_timing : cmd_timing_list ) {
                    bank_states_[rank][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
                }
            }
        }
    }
    return;
}

void ChannelState::UpdateOtherRanksTiming(int rank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto i = 0; i < config_.ranks; i++) {
        if(i != rank) {
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

void ChannelState::UpdateSameRankTiming(int rank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto j = 0; j < config_.bankgroups; j++) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[rank][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void ChannelState::IssueCommand(const Command& cmd, long clk) {
    cout << "Command Issue at clk = " << clk << " - "<< cmd << endl;
    UpdateState(cmd);
    UpdateTiming(cmd, clk);
    return;
}

void ChannelState::UpdateRefreshWaitingStatus(const Command& cmd, bool status) {
    if(cmd.cmd_type_ == CommandType::REFRESH) {
        for( auto j = 0; j < config_.bankgroups; j++) {
            for( auto k = 0; k < config_.banks_per_group; k++) {
                bank_states_[cmd.rank_][j][k]->UpdateRefreshWaitingStatus(status);
            }
        }
    }
    else if(cmd.cmd_type_ == CommandType::REFRESH_BANK) {
        for( auto k = 0; k < config_.banks_per_group; k++) {
            bank_states_[cmd.rank_][cmd.bankgroup_][k]->UpdateRefreshWaitingStatus(status);
        }
    }
    return;
}

bool ChannelState::ActivationConstraint(int rank, long curr_time) const {
    auto count = 0;
    for( auto act_time : activation_times_[rank]) {
        if(curr_time - act_time < config_.tFAW) //TODO Strict less than or less than or equal to?
            count++;
        else
            break; //Activation times are ordered. For simulator performance. Really needed?
    }
    return count >= config_.activation_window_depth; //ASSERT that count is never greater than activation_window_depth
}

void ChannelState::UpdateActivationTimes(int rank, long curr_time) {
    //Delete stale time entries
    for(auto list_itr = activation_times_[rank].begin(); list_itr != activation_times_[rank].end(); list_itr++) {
        auto act_time = *list_itr;
        if( curr_time - act_time > config_.tFAW) {
            activation_times_[rank].erase(list_itr, activation_times_[rank].end()); //ASSERT There will always be only one element to delete (+1 error - check)
            break;
        }
    }
    activation_times_[rank].push_front(curr_time);
    return;
}