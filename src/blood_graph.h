// Author: Tommy Jung

#ifndef __BLOOD_GRAPH_H
#define __BLOOD_GRAPH_H

#include <fstream>
#include <vector>
#include "common.h"
#include "configuration.h"
#include "command_queue.h"
#include "channel_state.h"

namespace dramsim3 {

class BloodGraph {

  public:
    BloodGraph(int channel_id, const Config &config);
    void ClockTick();
    void IsInRefresh(bool ref);
    void IssueCommand(const Command &cmd);
    CommandQueue* cmd_queue_;
    ChannelState* channel_state_;

  private:
    // channel info
    const Config &config_;
    int channel_id_;
    int clk_;
    int bank_count_;

    // refresh info
    bool is_in_ref_;
    int ref_count_;

    // bank info
    std::vector<bool> read_issued_;
    std::vector<bool> write_issued_;
    std::vector<int> pre_count_;
    std::vector<int> act_count_; 

    // trace stream
    std::ofstream trace_;
    void PrintTrace(int bank_id, const std::string &str);
};

} // namespace dramsim3

#endif
