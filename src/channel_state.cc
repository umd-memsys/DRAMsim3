#include "channel_state.h"

using namespace std;

ChannelState::ChannelState(int ranks, int bankgroups, int banks_per_group, const Timing& timing) :
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    bank_states_(ranks_, vector< vector<BankState*> >(bankgroups_, vector<BankState*>(banks_per_group_, NULL) ) ),
    activation_times_(ranks, list<long>()),
    timing_(timing)
{
    for(auto i = 0; i < ranks_; i++) {
        for(auto j = 0; j < bankgroups_; j++) {
            for(auto k = 0; k < banks_per_group; k++) {
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
            return Command(cmd, bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->GetRequiredCommandType(cmd));
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            //Static fixed order to check banks
            for(auto j = 0; j < bankgroups_; j++) {
                for(auto k = 0; k < banks_per_group_; k++) {
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
    switch(cmd.cmd_type_) {
        case CommandType::ACTIVATE:
            if(ActivationConstraint(cmd.rank_, clk))
                return false; //Note : no break here. Is this kind of coding good?
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
            for(auto j = 0; j < bankgroups_; j++) {
                for(auto k = 0; k < banks_per_group_; k++) {
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
            for(auto j = 0; j < bankgroups_; j++) {
                for(auto k = 0; k < banks_per_group_; k++) {
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
            UpdateActivationTimes(cmd.rank_, clk); //Note : no break here
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            //Same Bank
            UpdateSameBankTiming(cmd.rank_, cmd.bankgroup_, cmd.bank_, timing_.same_bank[int(cmd.cmd_type_)], clk);

            //Same Bankgroup other banks
            UpdateOtherBanksSameBankgroupTiming(cmd.rank_, cmd.bankgroup_, cmd.bank_, timing_.other_banks_same_bankgroup[int(cmd.cmd_type_)], clk);

            //Other bankgroups
            UpdateOtherBankgroupsSameRankTiming(cmd.rank_, cmd.bankgroup_, timing_.other_bankgroups_same_rank[int(cmd.cmd_type_)], clk);

            //Other ranks
            UpdateOtherRanksTiming(cmd.rank_, timing_.other_ranks[int(cmd.cmd_type_)], clk);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            UpdateSameRankTiming(cmd.rank_, timing_.same_rank[int(cmd.cmd_type_)], clk);
            break;
        default:
            exit(-1);
    }
    return ;
}

inline void ChannelState::UpdateSameBankTiming(int rank, int bankgroup, int bank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto cmd_timing : cmd_timing_list ) {
        bank_states_[rank][bankgroup][bank]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
    }
    return;
}

inline void ChannelState::UpdateOtherBanksSameBankgroupTiming(int rank, int bankgroup, int bank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto k = 0; k < banks_per_group_; k++) {
        if( k != bank) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[rank][bankgroup][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

inline void ChannelState::UpdateOtherBankgroupsSameRankTiming(int rank, int bankgroup, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto j = 0; j < bankgroups_; j++) {
        if(j != bankgroup) {
            for(auto k = 0; k < banks_per_group_; k++) {
                for(auto cmd_timing : cmd_timing_list ) {
                    bank_states_[rank][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
                }
            }
        }
    }
    return;
}

inline void ChannelState::UpdateOtherRanksTiming(int rank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto i = 0; i < ranks_; i++) {
        if(i != rank) {
            for(auto j = 0; j < bankgroups_; j++) {
                for(auto k = 0; k < banks_per_group_; k++) {
                    for(auto cmd_timing : cmd_timing_list ) {
                        bank_states_[i][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
                    }
                }
            }
        }
    }
    return;
}

inline void ChannelState::UpdateSameRankTiming(int rank, const list< pair<CommandType, int> >& cmd_timing_list, long clk) {
    for(auto j = 0; j < bankgroups_; j++) {
        for(auto k = 0; k < banks_per_group_; k++) {
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
        for( auto j = 0; j < bankgroups_; j++) {
            for( auto k = 0; k < banks_per_group_; k++) {
                bank_states_[cmd.rank_][j][k]->UpdateRefreshWaitingStatus(status);
            }
        }
    }
    else if(cmd.cmd_type_ == CommandType::REFRESH_BANK) {
        for( auto k = 0; k < banks_per_group_; k++) {
            bank_states_[cmd.rank_][cmd.bankgroup_][k]->UpdateRefreshWaitingStatus(status);
        }
    }
    return;
}

bool ChannelState::IsRefreshWaiting(int rank, int bankgroup, int bank) const {
    return bank_states_[rank][bankgroup][bank]->IsRefreshWaiting();
}

bool ChannelState::ActivationConstraint(int rank, long curr_time) const {
    auto count = 0;
    for( auto act_time : activation_times_[rank]) {
        if(curr_time - act_time < tFAW)
            count++;
        else
            break; //Activation times are ordered. For simulator performance. Really needed?
    }
    return count >= 4; //ASSERT that count is never greater than 4
}

void ChannelState::UpdateActivationTimes(int rank, long curr_time) {
    activation_times_[rank].push_front(curr_time);
    //Delete stale time entries
    for(auto list_itr = activation_times_[rank].begin(); list_itr != activation_times_[rank].end(); list_itr++) {
        auto act_time = *list_itr;
        if( curr_time - act_time > tFAW) {
            activation_times_[rank].erase(list_itr, activation_times_[rank].end()); //ASSERT There will always be only one element to delete (+1 error - check)
            break;
        }
    }
    return;
}

bool ChannelState::IsRowOpen(int rank, int bankgroup, int bank) const {
    return bank_states_[rank][bankgroup][bank]->IsRefreshWaiting();
}

int ChannelState::OpenRow(int rank, int bankgroup, int bank) const {
    return bank_states_[rank][bankgroup][bank]->OpenRow();
}

int ChannelState::RowHitCount(int rank, int bankgroup, int bank) const {
    return bank_states_[rank][bankgroup][bank]->RowHitCount();
}