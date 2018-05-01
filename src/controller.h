#ifndef __CONTROLLER_H
#define __CONTROLLER_H

#include <functional>
#include <list>
#include <map>
#include <vector>
#include "channel_state.h"
#include "command_queue.h"
#include "common.h"
#include "refresh.h"
#include "statistics.h"
#include "thermal.h"

namespace dramsim3 {

class Controller {
   public:
#ifdef THERMAL
    Controller(int channel, const Config &config, const Timing &timing,
               Statistics &stats, ThermalCalculator *thermalcalc,
               std::function<void(uint64_t)> read_callback,
               std::function<void(uint64_t)> write_callback);
#else
    Controller(int channel, const Config &config, const Timing &timing,
               Statistics &stats, std::function<void(uint64_t)> read_callback,
               std::function<void(uint64_t)> write_callback);
#endif  // THERMAL
    ~Controller(){};
    void ClockTick();
    bool WillAcceptTransaction();
    bool AddTransaction(Transaction trans);
    std::function<void(uint64_t)> read_callback_, write_callback_;
    int channel_id_;
    int QueueUsage() const;
    std::list<Command> issued_cmds;

   protected:
    uint64_t clk_;
    const Config &config_;
    ChannelState channel_state_;
    std::multimap<int, Transaction> pending_trans_;
    std::vector<Transaction> transaction_q_;
    std::vector<Transaction> return_queue_;
    CommandQueue cmd_queue_;
    Refresh refresh_;
    Statistics &stats_;
};
}  // namespace dramsim3
#endif
