#include <iostream>
#include "controller.h"
#include "timing.h"

using namespace std;

Controller::Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing) :
    timing_(timing),
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    bank_states_(ranks_, vector< vector<BankState*> >(bankgroups_, vector<BankState*>(banks_per_group_, NULL) ) )
{
    for(auto i = 0; i < ranks_; i++) {
        for(auto j = 0; j < bankgroups_; j++) {
            for(auto k = 0; k < banks_per_group; k++) {
                bank_states_[i][j][k] = new BankState(i, j, k);
            }
        }
    }
}

void Controller::UpdateTiming(const Command& cmd) {
    //Same Bank
    UpdateSameBank(cmd, timing_.same_bank[int(cmd.cmd_type_)]);

    //Same Bankgroup other banks
    UpdateOtherBanksSameBankgroup(cmd, timing_.other_banks_same_bankgroup[int(cmd.cmd_type_)]);

    //Other bankgroups
    UpdateOtherBankgroupsSameRank(cmd, timing_.other_bankgroups_same_rank[int(cmd.cmd_type_)]);

    //Other ranks
    UpdateOtherRanks(cmd, timing_.other_ranks[int(cmd.cmd_type_)]);

    return ;
}

void Controller::UpdateSameBank(const Command& cmd, const list< pair<CommandType, int> >& cmd_timing_list) {
    for(auto cmd_timing : cmd_timing_list ) {
        bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
    }
    return;
}

void Controller::UpdateOtherBanksSameBankgroup(const Command& cmd, const list< pair<CommandType, int> >& cmd_timing_list) {
    for(auto k = 0; k < banks_per_group_; k++) {
        if( k != cmd.bank_) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[cmd.rank_][cmd.bankgroup_][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

void Controller::UpdateOtherBankgroupsSameRank(const Command& cmd, const list< pair<CommandType, int> >& cmd_timing_list) {
    for(auto j = 0; j < bankgroups_; j++) {
        if(j != cmd.bankgroup_) {
            for(auto k = 0; k < banks_per_group_; k++) {
                for(auto cmd_timing : cmd_timing_list ) {
                    bank_states_[cmd.rank_][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
                }
            }
        }
    }
    return;
}

void Controller::UpdateOtherRanks(const Command& cmd, const list< pair<CommandType, int> >& cmd_timing_list) {
    for(auto i = 0; i < ranks_; i++) {
        if(i != cmd.rank_) {
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
