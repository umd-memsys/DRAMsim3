#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#include <vector>
#include <list>
#include "controller.h"
#include "common.h"


class Scheduler {
    public:
        Scheduler(const Controller& controller);
    private:
        const Controller& controller_;

        int next_rank_, next_bankgroup_, next_bank_;

        //Unified read write per bank queues
        std::vector< std::vector< std::vector< std::list<Request*> > > > req_q_;

        //Look at the request queues, timing requirements, state information etc
        //And decide the command to issue as per the scheduling policy
        Command GetCommandToIssue();

        //Check the queue sizes and insert the request and return true if able to do so
        bool InsertReq();

        //Delete the request and call any callback function in the CPU
        void DoneWithReq();

        void IterateNext();
};

#endif