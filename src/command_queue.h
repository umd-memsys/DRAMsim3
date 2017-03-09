#ifndef __COMMAND_QUEUE_H
#define __COMMAND_QUEUE_H

#include <vector>
#include <list>
#include "common.h"
#include "bankstate.h"

class CommandQueue {
    public:
        CommandQueue(int ranks, int bankgroups, int banks_per_group, std::vector< std::vector< std::vector<BankState*> > >* bank_states_ptr);
        Command GetCommandToIssue() ;
        Command GetRequiredCommand(const Command& cmd) const;
        bool IsReady(const Command& cmd) const;
        bool InsertReq(Request* req);
        long clk;
    private:
        int ranks_, bankgroups_, banks_per_group_;
        int next_rank_, next_bankgroup_, next_bank_;
        int size_q_;

        std::vector< std::vector< std::vector< std::list<Request*> > > > req_q_;
        const std::vector< std::vector< std::vector<BankState*> > >* bank_states_ptr_;

        void IterateNext();
};


#endif