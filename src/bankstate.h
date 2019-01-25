#ifndef __BANKSTATE_H
#define __BANKSTATE_H

#include <vector>
#include "common.h"

namespace dramsim3 {

class BankState {
   public:
    BankState();

    Command GetReadyCommand(const Command& cmd, uint64_t clk) const;

    // Update the state of the bank resulting after the execution of the command
    void UpdateState(const Command& cmd);

    // Update the existing timing constraints for the command
    void UpdateTiming(const CommandType cmd_type, uint64_t time);

    bool IsRowOpen() const { return state_ == State::OPEN; }
    int OpenRow() const { return open_row_; }
    int RowHitCount() const { return row_hit_count_; }

   private:
    // Current state of the Bank
    // Apriori or instantaneously transitions on a command.
    State state_;

    // Earliest time when the particular Command can be executed in this bank
    std::vector<uint64_t> cmd_timing_;

    // Currently open row
    // Applicable only if the bank is in OPEN state
    int open_row_;

    // Maximum allowed row hits to a bank before aggressively precharing it
    // To prevent starvation and allow fairness
    int row_hit_count_;
};

}  // namespace dramsim3
#endif
