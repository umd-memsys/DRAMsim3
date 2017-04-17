#ifndef __BANKSTATE_H
#define __BANKSTATE_H

#include <vector>
#include <list>
#include <utility>
#include "common.h"
#include "statistics.h"

namespace dramcore {

class BankState {
    public:
        BankState(Statistics &stats);

        //Get the command that needs to executed first to execute the comand of interest given the state of the bank
        CommandType GetRequiredCommandType(const Command& cmd);

        //Update the state of the bank resulting after the execution of the command
        void UpdateState(const Command& cmd);

        //Update the existing timing constraints for the command
        void UpdateTiming(const CommandType cmd_type, uint64_t time);

        //Check the timing constraints to see if the command can executed at the given time
        bool IsReady(CommandType cmd_type, uint64_t time) const { return time >= cmd_timing_[static_cast<int>(cmd_type)]; }
        
        void UpdateRefreshWaitingStatus(bool status) { refresh_waiting_ = status; return; }
        bool IsRefreshWaiting() const { return refresh_waiting_; }
        bool IsRowOpen() const { return state_ == State::OPEN; }
        int OpenRow() const { return open_row_; }
        int RowHitCount() const { return row_hit_count_; }
    private:
        Statistics& stats_;
        //Current state of the Bank
        //Apriori or instantaneously transitions on a command.
        State state_;

        // Earliest time when the particular Command can be executed in this bank
        std::vector<uint64_t> cmd_timing_;

        //Currently open row
        //Applicable only if the bank is in OPEN state
        int open_row_;

        //Maximum allowed row hits to a bank before aggressively precharing it
        //To prevent starvation and allow fairness
        int row_hit_count_;

        bool refresh_waiting_;

};

}
#endif
