#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <functional>
#include <map>
#include <vector>
#include "channel_state.h"
#include "command_queue.h"
#include "common.h"
#include "refresh.h"
#include "statistics.h"

#ifdef THERMAL
#include "thermal.h"
#endif  // THERMAL

namespace dramsim3 {

enum class RowBufPolicy { OPEN_PAGE, CLOSE_PAGE, SIZE };

class Controller {
   public:
#ifdef THERMAL
    Controller(int channel, const Config &config, const Timing &timing,
               Statistics &stats, ThermalCalculator& thermalcalc,
               std::function<void(uint64_t)> read_callback,
               std::function<void(uint64_t)> write_callback);
#else
    Controller(int channel, const Config &config, const Timing &timing,
               Statistics &stats, std::function<void(uint64_t)> read_callback,
               std::function<void(uint64_t)> write_callback);
#endif  // THERMAL
    ~Controller(){};
    void ClockTick();
    bool WillAcceptTransaction() const;
    bool AddTransaction(Transaction trans);
    std::function<void(uint64_t)> read_callback_, write_callback_;
    int channel_id_;
    int QueueUsage() const;

   protected:
    uint64_t clk_;
    const Config &config_;
    ChannelState channel_state_;
    CommandQueue cmd_queue_;
    Refresh refresh_;
    Statistics &stats_;

   private:
#ifdef THERMAL
    ThermalCalculator& thermal_calc_;
#endif  // THERMAL
    // queue that takes transactions from CPU side
    std::vector<Transaction> transaction_queue_;

    // transactions that are issued to command queue, use map for convenience
    std::multimap<int, Transaction> pending_queue_;
    
    // completed transactions
    std::vector<Transaction> return_queue_;

    // An ID that is used to keep track of commands in fly
    int cmd_id_;

    // the max number of cmds in fly, x2 to be safe
    const int max_cmd_id_;
    RowBufPolicy row_buf_policy_;
    void IssueCommand(const Command& tmp_cmd);
    void ProcessRWCommand(const Command& cmd);
    Command TransToCommand(const Transaction &trans);
};
}  // namespace dramsim3
#endif
