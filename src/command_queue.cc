#include "command_queue.h"
#include <vector>
#include <list>

using namespace std;

CommandQueue::CommandQueue(int ranks, int bankgroups, int banks_per_group, std::vector< std::vector< std::vector<BankState*> > >* bank_states_ptr) :
    clk(0),
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    next_rank_(0),
    next_bankgroup_(0),
    next_bank_(0),
    size_q_(16),
    req_q_(ranks, vector< vector<list<Request*>> >(bankgroups, vector<list<Request*>>(banks_per_group, list<Request*>()) ) ),
    bank_states_ptr_(bank_states_ptr)
{

}

Command CommandQueue::GetCommandToIssue() {
    //Rank, Bank, Bankgroup traversal of queues
    for(auto i = 0; i < ranks_; i++) {
        for(auto k = 0; k < banks_per_group_; k++) {
            for(auto j = 0; j < bankgroups_; j++) {
                list<Request*>& queue = req_q_[next_rank_][next_bankgroup_][next_bank_];
                IterateNext();
                //FRFCFS (First ready first come first serve)
                //Search the queue to pickup the first request whose required command could be issued this cycle
                for(auto itr = queue.begin(); itr != queue.end(); itr++) {
                    auto req = *itr;
                    Command cmd = GetRequiredCommand(req->cmd_);
                    if(IsReady(cmd)) {
                        if(req->cmd_.cmd_type_ == cmd.cmd_type_) {
                            //Sought of actually issuing the read/write command
                            queue.erase(itr);
                        }
                        return cmd;
                    }
                    //TODO - PreventRead write dependencies
                    //Having unified read write queues appears to a very dumb idea!
                }
            }
        }
    }
    return Command();
}

Command CommandQueue::GetRequiredCommand(const Command& cmd) const {
    auto bank_states = *bank_states_ptr_;
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            return Command(cmd, bank_states[cmd.rank_][cmd.bankgroup_][cmd.bank_]->GetRequiredCommandType(cmd));
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT:
            //Static fixed order to check banks
            for(auto j = 0; j < bankgroups_; j++) {
                for(auto k = 0; k < banks_per_group_; k++) {
                    CommandType required_cmd_type = bank_states[cmd.rank_][cmd.bankgroup_][cmd.bank_]->GetRequiredCommandType(cmd);
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

bool CommandQueue::IsReady(const Command& cmd) const {
    auto bank_states = *bank_states_ptr_;
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
        case CommandType::ACTIVATE:
        case CommandType::PRECHARGE:
        case CommandType::REFRESH_BANK:
            return bank_states[cmd.rank_][cmd.bankgroup_][cmd.bank_]->IsReady(cmd.cmd_type_, clk);
            break;
        case CommandType::REFRESH:
        case CommandType::SELF_REFRESH_ENTER:
        case CommandType::SELF_REFRESH_EXIT: {
            bool is_ready = true;
            for(auto j = 0; j < bankgroups_; j++) {
                for(auto k = 0; k < banks_per_group_; k++) {
                    is_ready &= bank_states[cmd.rank_][cmd.bankgroup_][cmd.bank_]->IsReady(cmd.cmd_type_, clk);
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

bool CommandQueue::InsertReq(Request* req) {
    auto r = req->cmd_.rank_, bg = req->cmd_.bankgroup_, b = req->cmd_.bank_;
    if( req_q_[r][bg][b].size() < size_q_ ) {
        req_q_[r][bg][b].push_back(req);
        return true;
    }
    else
        return false;
}


inline void CommandQueue::IterateNext() {
    next_bankgroup_ = (next_bankgroup_ + 1) % bankgroups_;
    if(next_bankgroup_ == 0) {
        next_bank_ = (next_bank_ + 1) % banks_per_group_;
        if(next_bank_ == 0) {
            next_rank_ = (next_rank_ + 1) % ranks_;
        }
    }
    return;
}