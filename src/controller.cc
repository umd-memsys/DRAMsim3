#include <iostream>
#include "controller.h"

using namespace std;

Controller::Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing) :
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    clk(0),
    channel_state_(ranks, bankgroups, banks_per_group, timing),
    cmd_queue_(ranks, bankgroups, banks_per_group, channel_state_)
{
}

void Controller::ClockTick() {
    Command cmd = cmd_queue_.GetCommandToIssue();
    if(cmd.IsValid()) {
        IssueCommand(cmd);
    }
    else {
        //Look for closing open banks if any. (Aggressive precharing)
    }
    clk++;
    cmd_queue_.clk++;
}

void Controller::IssueCommand(const Command& cmd) {
    cout << "Command Issue at clk = " << clk << " - "<< cmd << endl;
    channel_state_.UpdateState(cmd);
    channel_state_.UpdateTiming(cmd, clk);
    return;
}



bool Controller::InsertReq(Request* req) {
    return cmd_queue_.InsertReq(req);
}

