#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <functional>
#include <map>
#include <unordered_set>
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
               Statistics &stats, ThermalCalculator &thermalcalc,
               std::function<void(uint64_t)> read_callback,
               std::function<void(uint64_t)> write_callback);
#else
    Controller(int channel, const Config &config, const Timing &timing,
               Statistics &stats, std::function<void(uint64_t)> read_callback,
               std::function<void(uint64_t)> write_callback);
#endif  // THERMAL
    ~Controller(){};
    void ClockTick();
    bool WillAcceptTransaction(uint64_t hex_addr, bool is_write) const;
    bool AddTransaction(Transaction trans);
    std::function<void(uint64_t)> read_callback_, write_callback_;
    int channel_id_;
    int QueueUsage() const;

   private:
    uint64_t clk_;
    const Config &config_;
    ChannelState channel_state_;
    CommandQueue cmd_queue_;
    Refresh refresh_;
    Statistics &stats_;

#ifdef THERMAL
    ThermalCalculator &thermal_calc_;
#endif  // THERMAL
#ifdef GENERATE_TRACE
    std::ofstream cmd_trace_;
#endif  // GENERATE_TRACE
    // queue that takes transactions from CPU side
    bool is_unified_queue_;
    std::vector<Transaction> unified_queue_;
    std::vector<Transaction> read_queue_;
    std::vector<Transaction> write_queue_;
    std::unordered_set<uint64_t> in_write_buf_;

    // transactions that are issued to command queue, use map for convenience
    std::multimap<uint64_t, Transaction> pending_queue_;

    // completed transactions
    std::vector<Transaction> return_queue_;

    // row buffer policy
    RowBufPolicy row_buf_policy_;

    // transaction queueing
    int write_draining_;
    void ScheduleTransaction();
    void IssueCommand(const Command &tmp_cmd);
    Command TransToCommand(const Transaction &trans);
};
}  // namespace dramsim3
#endif
