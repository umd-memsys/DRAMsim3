#ifndef __BANKSTATE_H
#define __BANKSTATE_H

#include <vector>
#include <list>
#include <utility>
#include "common.h"

class BankState {
    public:
        BankState(int rank, int bankgroup, int bank);

        //Get the command that needs to executed first to execute the comand of interest given the state of the bank
        CommandType GetRequiredCommandType(const Command& cmd);

        //Update the state of the bank resulting after the execution of the command
        void UpdateState(const Command& cmd);

        //Update the existing timing constraints for the command
        void UpdateTiming(const CommandType cmd_type, long time);

        //Check the timing constraints to see if the command can executed at the given time
        bool IsReady(CommandType cmd_type, long time);

        void UpdateRefreshWaitingStatus(bool status);

        bool IsRefreshWaiting() const;

        bool IsRowOpen() const;

        int OpenRow() const;

        int RowHitCount() const;
    private:
        //Current state of the Bank
        //Apriori or instantaneously transitions on a command.
        State state_;

        // Earliest time when the particular Command can be executed in this bank
        std::vector<long> timing_;

        //Currently open row
        //Applicable only if the bank is in OPEN state
        int open_row_;

        //Maximum allowed row hits to a bank before aggressively precharing it
        //To prevent starvation and allow fairness
        int row_hit_count_;

        int refresh_waiting_;

        int rank_, bankgroup_, bank_;
};

#endif