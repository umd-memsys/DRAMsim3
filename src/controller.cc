#include <iostream>
#include "controller.h"

using namespace std;

Controller::Controller(int ranks, int bankgroups, int banks_per_group, const Timing& timing) :
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    clk(0),
    channel_state_(ranks, bankgroups, banks_per_group, timing),
    cmd_queue_(ranks, bankgroups, banks_per_group, channel_state_),
    refresh_(ranks, bankgroups, banks_per_group)
{
    printf("Creating a controller object with ranks = %d, bankgroups = %d, banks_per_group = %d", ranks_, bankgroups_, banks_per_group_);
}

void Controller::ClockTick() {
    //Refresh command is queued
    if( !refresh_.refresh_q_.empty()) {
        auto req = refresh_.refresh_q_.front();

        if(refresh_.PrepareForRefreshIssue())
            return;
    }

    //Read write queues
    Command cmd = cmd_queue_.GetCommandToIssue();
    if(cmd.IsValid()) {
        cout << "Command Issue at clk = " << clk << " - "<< cmd << endl;
        channel_state_.IssueCommand(cmd, clk);
    }
    else {
        //Look for closing open banks if any. (Aggressive precharing)
    }
    clk++;
    cmd_queue_.clk++;
}





bool Controller::InsertReq(Request* req) {
    return cmd_queue_.InsertReq(req);
}

