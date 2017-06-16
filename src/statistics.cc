#include "statistics.h"
#include "common.h"
#include "../ext/fmt/src/format.h"

using namespace std;
using namespace dramcore;

CounterStat::CounterStat(std::string name, std::string desc):
    BaseStat(name, desc),
    count_(0),
    last_epoch_count_(0)
{}

void CounterStat::Print(std::ostream& where) const {
    where << fmt::format("{:<40}{:^5}{:>10}{:>10}{}", name_, " = ", count_, " # ", description_) << endl;
    return;
}

void CounterStat::UpdateEpoch() {
    last_epoch_count_ = count_;
    return;
}

void CounterStat::PrintEpoch(std::ostream& where) const {
    //TODO - Think of ways to avoid code duplication - Currently CTRL+C,CTRL+V of Print with epoch subtraction
    where << fmt::format("{:<40}{:^5}{:>10}{:>10}{}", name_, " = ", count_ - last_epoch_count_, " # ", description_) << endl;
    return;
}

void CounterStat::PrintCSVHeader(std::ostream& where) const {
    where << fmt::format("{},", name_);
    return;
}

void CounterStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", count_ );
    return;
}

void CounterStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", count_ - last_epoch_count_);
    return;
}

HistogramStat::HistogramStat(int start, int end, uint32_t numb_bins, std::string name, std::string desc):
    BaseStat(name, desc),
    start_(start),
    end_(end),
    numb_bins_(numb_bins),
    neg_outlier_count_(0),
    pos_outlier_count_(0),
    last_epoch_neg_outlier_count_(0),
    last_epoch_pos_outlier_count_(0)
{
    if(start_ >= end_ || numb_bins <= 0) {
        cout << "Hisogram stat improperly specified" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
    bin_count_.resize(numb_bins);
    last_epoch_bin_count_.resize(numb_bins);
}

void HistogramStat::AddValue(int val) {
    if( val >= start_ && val <= end_) {
        auto bin_index = (val*numb_bins_)/(end_ - start_); //TODO - Could be made better
        bin_count_[bin_index]++;
    }
    else if( val < start_) {
        neg_outlier_count_++;
    }
    else if ( val > end_) {
        pos_outlier_count_++;
    }
    return;
}

void HistogramStat::Print(std::ostream& where) const {
    auto bin_width = (end_ - start_)/numb_bins_;
    where << fmt::format("{:<40}{:^5}{:>10}{:>10}{}", name_, " = ", " ", " # ", description_) << endl;
    auto bin_str = fmt::format("[ < {} ]", start_);
    where << fmt::format("{:^40}{:^5}{:>10}", bin_str, " = ", neg_outlier_count_) << endl;
    for(auto i = 0; i < numb_bins_; i++) {
        auto bin_start = start_ + i*bin_width;
        auto bin_end = start_ + (i+1)*bin_width - 1;
        auto bin_str = fmt::format("[ {}-{} ]", bin_start, bin_end);
        where << fmt::format("{:^40}{:^5}{:>10}", bin_str, " = ", bin_count_[i]) << endl;
    }
    bin_str = fmt::format("[ > {} ]", end_);
    where << fmt::format("{:^40}{:^5}{:>10}", bin_str, " = ", pos_outlier_count_) << endl;
    return;
}

void HistogramStat::UpdateEpoch() {
    last_epoch_neg_outlier_count_ = neg_outlier_count_;
    last_epoch_pos_outlier_count_ = pos_outlier_count_;
    for(auto i = 0; i < numb_bins_; i++) {
        last_epoch_bin_count_[i] = bin_count_[i];
    }
    return;
}

void HistogramStat::PrintEpoch(std::ostream& where) const {
    //TODO - Think of ways to avoid code duplication - Currently CTRL+C,CTRL+V of Print with epoch subtraction
    auto bin_width = (end_ - start_)/numb_bins_;
    where << fmt::format("{:<40}{:^5}{:>10}{:>10}{}", name_, " = ", " ", " # ", description_) << endl;
    auto bin_str = fmt::format("[ < {} ]", start_);
    where << fmt::format("{:^40}{:^5}{:>10}", bin_str, " = ", neg_outlier_count_ - last_epoch_neg_outlier_count_) << endl;
    for(auto i = 0; i < numb_bins_; i++) {
        auto bin_start = start_ + i*bin_width;
        auto bin_end = start_ + (i+1)*bin_width - 1;
        auto bin_str = fmt::format("[ {}-{} ]", bin_start, bin_end);
        where << fmt::format("{:^40}{:^5}{:>10}", bin_str, " = ", bin_count_[i] - last_epoch_bin_count_[i]) << endl;
    }
    bin_str = fmt::format("[ > {} ]", end_);
    where << fmt::format("{:^40}{:^5}{:>10}", bin_str, " = ", pos_outlier_count_ - last_epoch_pos_outlier_count_) << endl;
    return;
}


void HistogramStat::PrintCSVHeader(std::ostream& where) const {
    auto bin_width = (end_ - start_)/numb_bins_;
    where << fmt::format("{}[<{}],", name_, start_);
    for(auto i = 0; i < numb_bins_; i++) {
        auto bin_start = start_ + i*bin_width;
        auto bin_end = start_ + (i+1)*bin_width - 1;
        where << fmt::format("{}[{}-{}],", name_, bin_start, bin_end);
    }
    where << fmt::format("{}[>{}],", name_, end_);
    return;
}

void HistogramStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", neg_outlier_count_);
    for(auto i = 0; i < numb_bins_; i++) {
        where << fmt::format("{},", bin_count_[i]);
    }
    where << fmt::format("{},", pos_outlier_count_);
    return;
}


void HistogramStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", neg_outlier_count_ - last_epoch_neg_outlier_count_);
    for(auto i = 0; i < numb_bins_; i++) {
        where << fmt::format("{},", bin_count_[i] - last_epoch_bin_count_[i]);
    }
    where << fmt::format("{},", pos_outlier_count_ - last_epoch_pos_outlier_count_);
    return;
}
Statistics::Statistics():
    stats_list()
{
    //TODO - Should stats be global?
    numb_read_reqs_issued = CounterStat("numb_read_reqs_issued", "Number of read requests issued");
    numb_write_reqs_issued = CounterStat("numb_of_write_reqs_issued", "Number of write requests issued");
    numb_row_hits = CounterStat("numb_of_row_hits", "Number of row hits");
    numb_read_row_hits = CounterStat("numb_read_row_hits", "Number of read row hits");
    numb_write_row_hits = CounterStat("numb_write_row_hits", "Number of write row hits");
    numb_aggressive_precharges = CounterStat("numb_aggressive_precharges", "Number of aggressive precharges issued");
    numb_ondemand_precharges = CounterStat("numb_ondemand_precharges", "Number of on demand precharges issued");
    dramcycles = CounterStat("Cycles", "Total number of DRAM execution cycles");
    access_latency = HistogramStat(0, 80, 10, "access_latency", "Histogram of access latencies");
    numb_buffered_requests = CounterStat("numb_buffered_requests", "Number of buffered requests because queues were full");
    hbm_dual_command_issue_cycles = CounterStat("hbm_dual_command_issue_cycles", "Number of cycles in which two commands were issued");
    hbm_dual_non_rw_cmd_attempt_cycles = CounterStat("hbm_dual_non_rw_cmd_attempt_cycles", "Number of cycles during which an opportunity to issue a read/write is possibly missed");
    numb_read_cmds_issued = CounterStat("numb_read_cmds_issued", "Number of read commands issued");
    numb_write_cmds_issued = CounterStat("numb_write_cmds_issued", "Number of write commands issued");
    numb_activate_cmds_issued = CounterStat("numb_activate_cmds_issued", "Number of activate commands issued");
    numb_precharge_cmds_issued = CounterStat("numb_precharge_cmds_issued", "Number of precharge commands issued");
    numb_refresh_cmds_issued = CounterStat("numb_refresh_cmds_issued", "Number of refresh commands issued");
    numb_refresh_bank_cmds_issued = CounterStat("numb_refresh_bank_cmds_issued", "Number of refresh bank commands issued");
    numb_rw_rowhits_pending_refresh = CounterStat("numb_rw_rowhits_pending_refresh", "Number of read/write row hits issued while a refresh was pending");

    stats_list.push_back(&numb_read_reqs_issued);
    stats_list.push_back(&numb_write_reqs_issued);
    stats_list.push_back(&numb_row_hits);
    stats_list.push_back(&numb_read_row_hits);
    stats_list.push_back(&numb_write_row_hits);
    stats_list.push_back(&numb_aggressive_precharges);
    stats_list.push_back(&numb_ondemand_precharges);
    stats_list.push_back(&dramcycles);
    stats_list.push_back(&access_latency);
    stats_list.push_back(&numb_buffered_requests);
    stats_list.push_back(&hbm_dual_command_issue_cycles);
    stats_list.push_back(&hbm_dual_non_rw_cmd_attempt_cycles);
    stats_list.push_back(&numb_read_cmds_issued);
    stats_list.push_back(&numb_write_cmds_issued);
    stats_list.push_back(&numb_activate_cmds_issued);
    stats_list.push_back(&numb_precharge_cmds_issued);
    stats_list.push_back(&numb_refresh_cmds_issued);
    stats_list.push_back(&numb_refresh_bank_cmds_issued);
    stats_list.push_back(&numb_rw_rowhits_pending_refresh);
}


void Statistics::PrintStats(std::ostream &where) const {
    for(auto stat : stats_list) {
        stat->Print(where);
    }
    return;
}

void Statistics::UpdateEpoch() {
    for(auto stat : stats_list) {
        stat->UpdateEpoch();
    }
    return;
}

void Statistics::PrintEpochStats(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintEpoch(where);
    }
    return;
}


void Statistics::PrintStatsCSVHeader(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintCSVHeader(where);
    }
    where << endl;
    return;
}

void Statistics::PrintStatsCSVFormat(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintCSVFormat(where);
    }
    where << endl;
    return;
}

void Statistics::PrintEpochStatsCSVFormat(std::ostream& where) const {
    for(auto stat : stats_list) {
        stat->PrintEpochCSVFormat(where);
    }
    where << endl;
    return;
}

namespace dramcore {

ostream& operator<<(ostream& os, Statistics& stats) {
    stats.PrintStats(os);
    return os;
}

}