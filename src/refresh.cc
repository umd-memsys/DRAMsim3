#include "refresh.h"

using namespace std;

Refresh::Refresh(int ranks, int bankgroups, int banks_per_group) :
    clk(0),
    ranks_(ranks),
    bankgroups_(bankgroups),
    banks_per_group_(banks_per_group),
    last_bank_refresh_(ranks, vector< vector<long>>(bankgroups, vector<long>(banks_per_group, 0))),
    last_rank_refresh_(ranks, 0),
    next_rank_(0)
{}

void Refresh::ClockTick() {
    clk++;
    InsertRefresh();
    return;
}


void Refresh::InsertRefresh() {
    if( clk % tREFI == 0) {
        refresh_q_.push_back(new Request(CommandType::REFRESH, next_rank_));
        IterateNext();
    }
    return;
}


bool Refresh::PrepareForRefreshIssue() {
    //Currently assuming that we always issue the first request in the request queue necessarily.
    return true;
}

inline void Refresh::IterateNext() {
    next_rank_ = (next_rank_ + 1) % ranks_;
    return;
}