#ifndef __BANKSTATE_H
#define __BANKSTATE_H

#include <vector>
#include <list>
#include <utility>
#include "common.h"

class BankState {
    public:
        BankState(int rank, int bankgroup, int bank);
        CommandType GetRequiredCommand(const Request& req);
        void UpdateState(const Command& cmd);
        void UpdateTiming(const CommandType cmd_type, long time);
        bool IsReady(const Command& cmd, long time);
    private:
        // Current state of the Bank
        // Apriori or instantaneously transitions on a command.
        State state_;

        // Earliest time when the particular Command can be executed in this bank
        std::vector<long> timing_;

        //Currently open row
        //Applicable only if the bank is in OPEN state
        int open_row_;

        int rank_, bankgroup_, bank_;
};

#endif