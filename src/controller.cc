#include <iostream>
#include "controller.h"

using namespace std;

Controller::Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing) :
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    clk(0),
    timing_(timing),
    req_q_(ranks_, vector< vector<list<Request*>> >(bankgroups_, vector<list<Request*>>(banks_per_group_, list<Request*>()) ) ),
    bank_states_(ranks_, vector< vector<BankState*> >(bankgroups_, vector<BankState*>(banks_per_group_, NULL) ) ),
    next_rank_(0),
    next_bankgroup_(0),
    next_bank_(0),
    size_q(16)
{
    for(auto i = 0; i < ranks_; i++) {
        for(auto j = 0; j < bankgroups_; j++) {
            for(auto k = 0; k < banks_per_group; k++) {
                bank_states_[i][j][k] = new BankState(i, j, k);
            }
        }
    }
}

void Controller::ClockTick() {
    Command cmd = GetCommandToIssue();
    if(cmd.IsValid()) {
        IssueCommand(cmd);
    }
    else {
        //Look for closing open banks if any. (Aggressive precharing)
    }
    clk++;
}

Command Controller::GetCommandToIssue() {
    //Rank, Bank, Bankgroup traversal of queues
    for(auto i = 0; i < ranks_; i++) {
        for(auto k = 0; k < banks_per_group_; k++) {
            for(auto j = 0; j < bankgroups_; j++) {
                const list<Request*>& queue = req_q_[next_rank_][next_bankgroup_][next_bank_];
                IterateNext();
                //FRFCFS (First ready first come first serve)
                //Search the queue to pickup the first request whose required command could be issued this cycle
                for(auto req : queue) {
                    Command cmd = GetRequiredCommand(req->cmd_);
                    if(IsReady(cmd)) 
                        return cmd;
                    //TODO - PreventRead write dependencies
                    //Having unified read write queues appears to a very dumb idea!
                }
            }
        }
    }
    return Command();
}

void Controller::IssueCommand(const Command& cmd) {
    UpdateState(cmd);
    UpdateTiming(cmd);
    return;
}

Command Controller::GetRequiredCommand(const Command& cmd) const {
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
                    CommandType required_cmd_type = bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->GetRequiredCommandType(cmd);
                    if( required_cmd_type != cmd.cmd_type_)
                        return Command(cmd, required_cmd_type);
                }
            }
            return cmd;
            break;
        default:
            exit(-1);
    }
}

bool Controller::IsReady(const Command& cmd) const {
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
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
                    is_ready &= bank_states_[cmd.rank_][cmd.bankgroup_][cmd.bank_]->IsReady(cmd.cmd_type_, clk);
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


void Controller::UpdateState(const Command& cmd) {
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

void Controller::UpdateTiming(const Command& cmd) {
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            //Same Bank
            UpdateSameBankTiming(cmd.rank_, cmd.bankgroup_, cmd.bank_, timing_.same_bank[int(cmd.cmd_type_)]);

            //Same Bankgroup other banks
            UpdateOtherBanksSameBankgroupTiming(cmd.rank_, cmd.bankgroup_, cmd.bank_, timing_.other_banks_same_bankgroup[int(cmd.cmd_type_)]);

            //Other bankgroups
            UpdateOtherBankgroupsSameRankTiming(cmd.rank_, cmd.bankgroup_, timing_.other_bankgroups_same_rank[int(cmd.cmd_type_)]);

            //Other ranks
            UpdateOtherRanksTiming(cmd.rank_, timing_.other_ranks[int(cmd.cmd_type_)]);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            UpdateSameRankTiming(cmd.rank_, timing_.same_rank[int(cmd.cmd_type_)]);
            break;
        default:
            exit(-1);
    }
    return ;
}

bool Controller::InsertReq(Request* req) {
    auto r = req->cmd_.rank_, bg = req->cmd_.bankgroup_, b = req->cmd_.bank_;
    if( req_q_[r][bg][b].size() < size_q ) {
        req_q_[r][bg][b].push_back(req);
        return true;
    }
    else
        return false;
}

inline void Controller::UpdateSameBankTiming(int rank, int bankgroup, int bank, const list< pair<CommandType, int> >& cmd_timing_list) {
    for(auto cmd_timing : cmd_timing_list ) {
        bank_states_[rank][bankgroup][bank]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
    }
    return;
}

inline void Controller::UpdateOtherBanksSameBankgroupTiming(int rank, int bankgroup, int bank, const list< pair<CommandType, int> >& cmd_timing_list) {
    for(auto k = 0; k < banks_per_group_; k++) {
        if( k != bank) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[rank][bankgroup][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

inline void Controller::UpdateOtherBankgroupsSameRankTiming(int rank, int bankgroup, const list< pair<CommandType, int> >& cmd_timing_list) {
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

inline void Controller::UpdateOtherRanksTiming(int rank, const list< pair<CommandType, int> >& cmd_timing_list) {
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

inline void Controller::UpdateSameRankTiming(int rank, const list< pair<CommandType, int> >& cmd_timing_list) {
    for(auto j = 0; j < bankgroups_; j++) {
        for(auto k = 0; k < banks_per_group_; k++) {
            for(auto cmd_timing : cmd_timing_list ) {
                bank_states_[rank][j][k]->UpdateTiming(cmd_timing.first, clk + cmd_timing.second);
            }
        }
    }
    return;
}

inline void Controller::IterateNext() {
    next_bankgroup_ = (next_bankgroup_ + 1) % bankgroups_;
    if(next_bankgroup_ == 0) {
        next_bank_ = (next_bank_ + 1) % banks_per_group_;
        if(next_bank_ == 0) {
            next_rank_ = (next_rank_ + 1) % ranks_;
        }
    }
    return;
}