#include "scheduler.h"

using namespace std;

Scheduler::Scheduler(const Controller& controller) :
    controller_(controller),
    req_q_(controller.ranks_, vector< vector<list<Request*>> >(controller.bankgroups_, vector<list<Request*>>(controller.banks_per_group_, list<Request*>()) ) )
{}

Command Scheduler::GetCommandToIssue() {
    //Rank, Bank, Bankgroup traversal of queues
    for(auto i = 0; i < controller_.ranks_; i++) {
        for(auto k = 0; k < controller_.banks_per_group_; k++) {
            for(auto j = 0; j < controller_.bankgroups_; j++) {
                const list<Request*>& queue = req_q_[next_rank_][next_bankgroup_][next_bank_];
                IterateNext();
                //FRFCFS (First ready first come first serve)
                //Search the queue to pickup the first request whose required command could be issued this cycle
                for(auto req : queue) {
                    Command cmd = controller_.GetRequiredCommand(req->cmd_);
                    if(controller_.IsReady(cmd)) 
                        return cmd;
                    //TODO - PreventRead write dependencies
                    //Having unified read write queues appears to a very dumb idea!
                }
            }
        }
    }
    return Command();
}

void Scheduler::IterateNext() {
    next_bankgroup_ = (next_bankgroup_ + 1) % controller_.bankgroups_;
    if(next_bankgroup_ == 0) {
        next_bank_ = (next_bank_ + 1) % controller_.banks_per_group_;
        if(next_bank_ == 0) {
            next_rank_ = (next_rank_ + 1) % controller_.ranks_;
        }
    }
    return;
}