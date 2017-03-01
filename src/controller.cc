#include "controller.h"

using namespace std;

Controller::Controller(int ranks, int bankgroups, int banks_per_group) :
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
    switch(cmd.cmd_type_) {
        case CommandType::READ: {
            //CommandType::READ
            UpdateSameBankgroup(cmd, CommandType::READ, clk + max(tBurst, tCCDL));
            UpdateOtherBankgroups(cmd, CommandType::READ, clk + max(tBurst, tCCDS));
            UpdateOtherRanks(cmd, CommandType::READ, clk + tBurst + tRTRS);

            //CommandType::WRITE

            break;
        }
        case CommandType::READ_PRECHARGE: {
            break;
        }
        case CommandType::WRITE: {
            break;
        }
        case CommandType::WRITE_PRECHARGE: {
            break;
        }
        case CommandType::ACTIVATE: {
            break;
        }
        case CommandType::PRECHARGE: {
            break;
        }
        case CommandType::REFRESH: {
            break;
        }
        case CommandType::SELF_REFRESH_ENTER: {
            break;
        }
        case CommandType::SELF_REFRESH_EXIT: {
            break;
        }
        default:
            exit(-1);
    }
    return ;
}


void Controller::UpdateSameBank(const Command& cmd, CommandType cmd_type, long time) {
    bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->UpdateTiming(cmd_type, time);
    return;
}

void Controller::UpdateOtherBanksSameBankgroup(const Command& cmd, CommandType cmd_type, long time) {
    for(auto k = 0; k < banks_per_group_; k++) {
        if( k != cmd.bank_)
            bank_states_[cmd.rank_][cmd.bankgroup_][k]->UpdateTiming(cmd_type, time);
    }
    return;
}

void Controller::UpdateSameBankgroup(const Command& cmd, CommandType cmd_type, long time) {
    for(auto k = 0; k < banks_per_group_; k++)
        bank_states_[cmd.rank_][cmd.bankgroup_][k]->UpdateTiming(cmd_type, time);
    return;
}

void Controller::UpdateOtherBankgroups(const Command& cmd, CommandType cmd_type, long time) {
    for(auto j = 0; j < bankgroups_; j++) {
        if(j != cmd.bankgroup_) {
            for(auto k = 0; k < banks_per_group_; k++) {
                bank_states_[cmd.rank_][j][k]->UpdateTiming(cmd_type, time);
            }
        }
    }
    return;
}

void Controller::UpdateOtherRanks(const Command& cmd, CommandType cmd_type, long time) {
    for(auto i = 0; i < ranks_; i++) {
        if(i != cmd.rank_) {
            for(auto j = 0; j < bankgroups_; j++) {
                for(auto k = 0; k < banks_per_group_; k++) {
                    bank_states_[i][j][k]->UpdateTiming(cmd_type, time);
                }
            }

        }
    }
}