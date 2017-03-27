#include "channel_state.h"

using namespace std;

ChannelState::ChannelState(const Config& config, const Timing& timing) :
    config_(config),
    timing_(timing),
    bank_states_(config_.ranks, std::vector< std::vector<BankState*> >(config_.bankgroups, std::vector<BankState*>(config_.banks_per_group, NULL) ) ),
    activation_times_(config_.ranks, std::list<long>())
{
    for(auto i = 0; i < config_.ranks; i++) {
        for(auto j = 0; j < config_.bankgroups; j++) {
            for(auto k = 0; k < config_.banks_per_group; k++) {
                bank_states_[i][j][k] = new BankState();
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
            exit(-1);
    }
}

bool ChannelState::IsReady(const Command& cmd, long clk) const {
    if(cmd.cmd_type_ != CommandType::REFRESH && cmd.cmd_type_ != CommandType::PRECHARGE && IsRefreshWaiting(cmd.Rank(), cmd.Bankgroup(), cmd.Bank()))
        return false;
    switch(cmd.cmd_type_) {
        case CommandType::ACTIVATE:
            if(ActivationConstraint(cmd.Rank(), clk))
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
            bank_states_[cmd.Rank()][cmd.Bankgroup()][cmd.Bank()]->UpdateState(cmd);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            for(auto j = 0; j < config_.bankgroups; j++) {
                for(auto k = 0; k < config_.banks_per_group; k++) {
                    bank_states_[cmd.Rank()][j][k]->UpdateState(cmd);
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
            exit(-1);
    }
    return ;
}

void ChannelState::UpdateSameBankTiming(const Address& addr, const std::list< std::pair<CommandType, unsigned int> >& cmd_timing_list, long clk) {
    for(auto cmd_timing : cmd_timing_list ) {
        bank_states_[addr.rank_][addr.bankgroup_][addr.bank_]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
    }
    return;
}

void ChannelState::UpdateOtherBanksSameBankgroupTiming(const Address& addr, const std::list< std::pair<CommandType, unsigned int> >& cmd_timing_list, long clk) {
    for(auto k = 0; k < config_.banks_per_group; k++) {
        if( k != addr.bank_) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[addr.rank_][addr.bankgroup_][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void ChannelState::UpdateOtherBankgroupsSameRankTiming(const Address& addr, const std::list< std::pair<CommandType, unsigned int> >& cmd_timing_list, long clk) {
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

void ChannelState::UpdateOtherRanksTiming(const Address& addr, const std::list< std::pair<CommandType, unsigned int> >& cmd_timing_list, long clk) {
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

void ChannelState::UpdateSameRankTiming(const Address& addr, const std::list< std::pair<CommandType, unsigned int> >& cmd_timing_list, long clk) {
    for(auto j = 0; j < config_.bankgroups; j++) {
        for(auto k = 0; k < config_.banks_per_group; k++) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[addr.rank_][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
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
                bank_states_[cmd.Rank()][j][k]->UpdateRefreshWaitingStatus(status);
            }
        }
    }
    else if(cmd.cmd_type_ == CommandType::REFRESH_BANK) {
        for( auto k = 0; k < config_.banks_per_group; k++) {
            bank_states_[cmd.Rank()][cmd.Bankgroup()][k]->UpdateRefreshWaitingStatus(status);
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