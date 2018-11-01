#include "statistics.h"
#include "../ext/fmt/src/format.h"
#include "common.h"

namespace dramsim3 {

template <class T>
void PrintNameValue(std::ostream& where, std::string name, T value) {
    where << fmt::format("{:^30}{:^5}{:>12}", name, " = ", value) << std::endl;
    return;
}

template <class T>
void PrintNameValueDesc(std::ostream& where, std::string name, T value,
                        std::string description) {
    // not making this a class method because we need to calculate
    // power & bw later, which are not BaseStat members
    where << fmt::format("{:<30}{:^3}{:>12}{:>5}{}", name, " = ", value, " # ",
                         description)
          << std::endl;
    return;
}

CounterStat::CounterStat(std::string name, std::string desc)
    : BaseStat(name, desc), count_(0), last_epoch_count_(0) {}

void CounterStat::Print(std::ostream& where) const {
    PrintNameValueDesc(where, name_, count_, description_);
    return;
}

void CounterStat::UpdateEpoch() {
    last_epoch_count_ = count_;
    return;
}

void CounterStat::PrintEpoch(std::ostream& where) const {
    PrintNameValueDesc(where, name_, count_ - last_epoch_count_, description_);
    return;
}

void CounterStat::PrintCSVHeader(std::ostream& where) const {
    where << fmt::format("{},", name_);
    return;
}

void CounterStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", count_);
    return;
}

void CounterStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", count_ - last_epoch_count_);
    return;
}

DoubleStat::DoubleStat(double inc, std::string name, std::string desc)
    : BaseStat(name, desc), value(0.0), last_epoch_value(0.0), inc_(inc) {}

void DoubleStat::Print(std::ostream& where) const {
    PrintNameValueDesc(where, name_, value, description_);
    return;
}

void DoubleStat::UpdateEpoch() {
    last_epoch_value = value;
    return;
}

void DoubleStat::PrintEpoch(std::ostream& where) const {
    PrintNameValueDesc(where, name_, value - last_epoch_value, description_);
    return;
}

void DoubleStat::PrintCSVHeader(std::ostream& where) const {
    where << fmt::format("{},", name_);
    return;
}

void DoubleStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", value);
    return;
}

void DoubleStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", value - last_epoch_value);
    return;
}

DoubleComputeStat::DoubleComputeStat(std::string name, std::string desc)
    : BaseStat(name, desc), epoch_value(0), cumulative_value(0) {}

void DoubleComputeStat::Print(std::ostream& where) const {
    PrintNameValueDesc(where, name_, cumulative_value, description_);
    return;
}

void DoubleComputeStat::UpdateEpoch() {}

void DoubleComputeStat::PrintEpoch(std::ostream& where) const {
    PrintNameValueDesc(where, name_, epoch_value, description_);
    return;
}

void DoubleComputeStat::PrintCSVHeader(std::ostream& where) const {
    where << fmt::format("{},", name_);
    return;
}

void DoubleComputeStat::PrintCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", cumulative_value);
    return;
}

void DoubleComputeStat::PrintEpochCSVFormat(std::ostream& where) const {
    where << fmt::format("{},", epoch_value);
    return;
}

HistogramStat::HistogramStat(int start, int end, int numb_bins,
                             std::string name, std::string desc)
    : BaseStat(name, desc),
      start_(start),
      end_(end),
      numb_bins_(numb_bins),
      epoch_count_(0) {
    if (start_ >= end_ || numb_bins <= 0) {
        std::cout << "Histogram stat improperly specified" << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }
}

void HistogramStat::AddValue(int val) {
    if (bins_.count(val) <= 0) {
        bins_[val] = 1;
    } else {
        bins_[val] += 1;
    }
}

std::vector<uint64_t> HistogramStat::GetAggregatedBins() const {
    int bin_width = (end_ - start_) / numb_bins_;
    // 2 extra slots to insert positive and negative outliers
    std::vector<uint64_t> aggr_bins(numb_bins_ + 2, 0);
    for (auto i = bins_.begin(); i != bins_.end(); i++) {
        if (i->first < start_) {
            aggr_bins.front() += i->second;
        } else if (i->first > end_) {
            aggr_bins.back() += i->second;
        } else {
            int bin_index = (i->first - start_) / bin_width +
                            1;  // +1 to offset the negative outlier
            aggr_bins[bin_index] += i->second;
        }
    }
    return aggr_bins;
}

uint64_t HistogramStat::AccuSum() const {
    uint64_t sum = 0;
    for (auto i = bins_.begin(); i != bins_.end(); i++) {
        sum += i->first * i->second;
    }
    return sum;
}

uint64_t HistogramStat::CountSum() const {
    uint64_t count = 0;
    for (auto i = bins_.begin(); i != bins_.end(); i++) {
        count += i->second;
    }
    return count;
}

double HistogramStat::GetAverage() const {
    uint64_t sum = AccuSum();
    uint64_t count = CountSum();
    return static_cast<double>(sum) / static_cast<double>(count);
}

void HistogramStat::Print(std::ostream& where) const {
    // ONLY print aggregated histograms in the final text output
    PrintNameValueDesc(where, name_, " ", description_);
    std::vector<uint64_t> aggr_bins = GetAggregatedBins();

    // get strings
    std::vector<std::string> bin_strs;
    std::string bin_str = fmt::format("[ < {} ]", start_);
    bin_strs.push_back(bin_str);
    int bin_width = (end_ - start_) / numb_bins_;
    for (int i = 1; i < numb_bins_ + 1; i++) {
        auto bin_start = start_ + (i - 1) * bin_width;
        auto bin_end = start_ + i * bin_width - 1;
        bin_str = fmt::format("[ {}-{} ]", bin_start, bin_end);
        bin_strs.push_back(bin_str);
    }
    bin_str = fmt::format("[ > {} ]", end_);
    bin_strs.push_back(bin_str);

    for (unsigned i = 0; i < aggr_bins.size(); i++) {
        PrintNameValue(where, bin_strs[i], aggr_bins[i]);
    }
    return;
}

void HistogramStat::UpdateEpoch() {
    last_epoch_bins_ = bins_;
    epoch_count_++;
    return;
}

void HistogramStat::PrintCSVHeader(std::ostream& where) const {
    for (auto i = bins_.begin(); i != bins_.end(); i++) {
        where << fmt::format("{}-{},", name_, i->first);
    }
    return;
}

void HistogramStat::PrintCSVFormat(std::ostream& where) const {
    for (auto i = bins_.begin(); i != bins_.end(); i++) {
        where << fmt::format("{},", i->second);
    }
    return;
}

void HistogramStat::PrintEpochCSVFormat(std::ostream& where) const {
    for (auto i = bins_.begin(); i != bins_.end(); i++) {
        where << fmt::format("{},{},{},{}", name_, i->first, i->second,
                             epoch_count_)
              << std::endl;
    }
    return;
}

Statistics::Statistics(const Config& config, int channel_id)
    : stats_list(), config_(config), channel_id_(channel_id), last_clk_(0) {
    num_reads_done =
        CounterStat("num_reads_done", "Number of read requests issued");
    num_writes_done =
        CounterStat("num_writes_done", "Number of write requests issued");
    num_write_buf_hits =
        CounterStat("num_write_buf_hits", "Number of write buffer hits");
    hmc_reqs_done = CounterStat("hmc_reqs_done", "HMC Requests finished");
    num_row_hits = CounterStat("num_row_hits", "Number of row hits");
    num_read_row_hits =
        CounterStat("num_read_row_hits", "Number of read row hits");
    num_write_row_hits =
        CounterStat("num_write_row_hits", "Number of write row hits");
    num_ondemand_pres = CounterStat("num_ondemand_pres",
                                    "Number of on demand precharges issued");
    dramcycles = CounterStat("cycles", "Total number of DRAM execution cycles");
    hbm_dual_cmds = CounterStat(
        "hbm_dual_cmds", "Number of cycles in which two commands were issued");
    num_read_cmds =
        CounterStat("num_read_cmds", "Number of read commands issued");
    num_write_cmds =
        CounterStat("num_write_cmds", "Number of write commands issued");
    num_act_cmds =
        CounterStat("num_act_cmds", "Number of activate commands issued");
    num_pre_cmds =
        CounterStat("num_pre_cmds", "Number of precharge commands issued");
    num_refresh_cmds =
        CounterStat("num_refresh_cmds", "Number of refresh commands issued");
    num_refb_cmds =
        CounterStat("num_refb_cmds", "Number of refresh bank commands issued");
    num_sref_enter_cmds =
        CounterStat("num_sref_enter_cmds",
                    "Number of self-refresh mode enter commands issued");
    num_sref_exit_cmds =
        CounterStat("num_sref_exit_cmds",
                    "Number of self-refresh mode exit commands issued");
    num_wr_dependency =
        CounterStat("num_wr_dependency", "Number of W after R dependency");
    Init2DStats(sref_cycles, config_.channels, config_.ranks, "channel", "rank",
                "sref_cycles", "Cycles in self-refresh state");
    Init2DStats(all_bank_idle_cycles, config_.channels, config_.ranks,
                "channel", "rank", "all_bank_idle_cycles",
                "Cycles of all banks are idle");
    Init2DStats(active_cycles, config_.channels, config_.ranks, "channel",
                "rank", "rank_active_cycles",
                "Number of cycles the rank ramains active");
    act_energy = DoubleComputeStat("act_energy", "ACT energy");
    read_energy =
        DoubleComputeStat("read_energy", "READ energy (not including IO)");
    write_energy =
        DoubleComputeStat("write_energy", "WRITE energy (not including IO)");
    ref_energy = DoubleComputeStat("ref_energy", "Refresh energy");
    refb_energy = DoubleComputeStat("refb_energy", "Bank-Refresh energy");
    Init2DStats(act_stb_energy, config_.channels, config_.ranks, "channel",
                "rank", "act_stb_energy", "Active standby Energy");
    Init2DStats(pre_stb_energy, config_.channels, config_.ranks, "channel",
                "rank", "pre_stb_energy", "Precharge standby energy");
    Init2DStats(pre_pd_energy, config_.channels, config_.ranks, "channel",
                "rank", "pre_pd_energy", "Precharge powerdown energy");
    Init2DStats(sref_energy, config_.channels, config_.ranks, "channel", "rank",
                "sref_energy", "Self-refresh energy");
    total_energy =
        DoubleComputeStat("total_energy", "(pJ) Total energy consumed");
    queue_usage =
        DoubleComputeStat("queue_usage", "average overall command queue usage");
    average_power = DoubleComputeStat("average_power",
                                      "(mW) Average Power for all devices");
    average_bandwidth = DoubleComputeStat("average_bandwidth",
                                          "(GB/s) Average Aggregate Bandwidth");
    average_latency =
        DoubleComputeStat("average_latency", "Average latency in DRAM cycles");
    average_interarrival = DoubleComputeStat(
        "average_interarrival", "Average interarrival latency of requests");
    // histogram stats
    access_latency = HistogramStat(0, 200, 10, "access_latency",
                                   "Histogram of access latencies");
    interarrival_latency = HistogramStat(0, 100, 10, "interarrival_latency",
                                         "Histogram of interarrival latencies");

    stats_list.push_back(&num_reads_done);
    stats_list.push_back(&num_writes_done);
    stats_list.push_back(&num_write_buf_hits);
    stats_list.push_back(&hmc_reqs_done);
    stats_list.push_back(&num_row_hits);
    stats_list.push_back(&num_read_row_hits);
    stats_list.push_back(&num_write_row_hits);
    stats_list.push_back(&num_ondemand_pres);
    stats_list.push_back(&dramcycles);
    stats_list.push_back(&hbm_dual_cmds);
    stats_list.push_back(&num_read_cmds);
    stats_list.push_back(&num_write_cmds);
    stats_list.push_back(&num_act_cmds);
    stats_list.push_back(&num_pre_cmds);
    stats_list.push_back(&num_refresh_cmds);
    stats_list.push_back(&num_refb_cmds);
    stats_list.push_back(&num_sref_enter_cmds);
    stats_list.push_back(&num_sref_exit_cmds);
    stats_list.push_back(&num_wr_dependency);
    stats_list.push_back(&act_energy);
    stats_list.push_back(&read_energy);
    stats_list.push_back(&write_energy);
    stats_list.push_back(&ref_energy);
    stats_list.push_back(&refb_energy);

    // push vectorized stats to list
    Push2DStatsToList(all_bank_idle_cycles);
    Push2DStatsToList(active_cycles);
    Push2DStatsToList(sref_cycles);
    Push2DStatsToList(act_stb_energy);
    Push2DStatsToList(pre_stb_energy);
    Push2DStatsToList(pre_pd_energy);
    Push2DStatsToList(sref_energy);
    stats_list.push_back(&total_energy);
    stats_list.push_back(&queue_usage);
    stats_list.push_back(&average_power);
    stats_list.push_back(&average_bandwidth);
    stats_list.push_back(&average_latency);
    stats_list.push_back(&average_interarrival);
    histo_stats_list.push_back(&access_latency);
    histo_stats_list.push_back(&interarrival_latency);
}

void Statistics::PreEpochCompute(uint64_t clk) {
    // because HMC requests != read commands,
    uint64_t reqs_issued_epoch, reqs_issued;
    if (!config_.IsHMC()) {
        reqs_issued_epoch =
            num_reads_done.Count() - num_reads_done.LastCount() +
            num_writes_done.Count() - num_writes_done.LastCount();
        reqs_issued = num_reads_done.Count() + num_writes_done.Count();
    } else {
        reqs_issued_epoch = hmc_reqs_done.Count() - hmc_reqs_done.LastCount();
        reqs_issued = hmc_reqs_done.Count();
    }

    // Epoch level compute stats
    act_energy.epoch_value = (num_act_cmds.Count() - num_act_cmds.LastCount()) *
                             config_.act_energy_inc;
    read_energy.epoch_value =
        (num_read_cmds.Count() - num_read_cmds.LastCount()) *
        config_.read_energy_inc;
    write_energy.epoch_value =
        (num_write_cmds.Count() - num_write_cmds.LastCount()) *
        config_.write_energy_inc;
    ref_energy.epoch_value =
        (num_refresh_cmds.Count() - num_refresh_cmds.LastCount()) *
        config_.ref_energy_inc;
    refb_energy.epoch_value =
        (num_refb_cmds.Count() - num_refb_cmds.LastCount()) *
        config_.refb_energy_inc;
    for (int i = 0; i < config_.channels; i++) {
        for (int j = 0; j < config_.ranks; j++) {
            act_stb_energy[i][j].epoch_value =
                (active_cycles[i][j].Count() -
                 active_cycles[i][j].LastCount()) *
                config_.act_energy_inc;
            pre_stb_energy[i][j].epoch_value =
                (all_bank_idle_cycles[i][j].Count() -
                 all_bank_idle_cycles[i][j].LastCount()) *
                config_.pre_stb_energy_inc;
            sref_energy[i][j].epoch_value =
                (sref_cycles[i][j].Count() - sref_cycles[i][j].LastCount()) *
                config_.sref_energy_inc;
        }
    }
    total_energy.epoch_value =
        act_energy.epoch_value + read_energy.epoch_value +
        write_energy.epoch_value + ref_energy.epoch_value +
        refb_energy.epoch_value + Stats2DEpochSum(act_stb_energy) +
        Stats2DEpochSum(pre_stb_energy) + Stats2DEpochSum(pre_pd_energy) +
        Stats2DEpochSum(sref_energy);
    average_power.epoch_value = total_energy.epoch_value / (clk - last_clk_);
    average_bandwidth.epoch_value =
        (reqs_issued_epoch * config_.request_size_bytes) /
        ((clk - last_clk_) * config_.tCK);

    // cumulative compute stats
    act_energy.cumulative_value = num_act_cmds.Count() * config_.act_energy_inc;
    read_energy.cumulative_value =
        num_read_cmds.Count() * config_.read_energy_inc;
    write_energy.cumulative_value =
        num_write_cmds.Count() * config_.write_energy_inc;
    ref_energy.cumulative_value =
        num_refresh_cmds.Count() * config_.ref_energy_inc;
    refb_energy.cumulative_value =
        num_refb_cmds.Count() * config_.refb_energy_inc;
    for (int i = 0; i < config_.channels; i++) {
        for (int j = 0; j < config_.ranks; j++) {
            act_stb_energy[i][j].cumulative_value =
                active_cycles[i][j].Count() * config_.act_stb_energy_inc;
            pre_stb_energy[i][j].cumulative_value =
                all_bank_idle_cycles[i][j].Count() * config_.pre_stb_energy_inc;
            sref_energy[i][j].cumulative_value =
                sref_cycles[i][j].Count() * config_.sref_energy_inc;
        }
    }
    total_energy.cumulative_value =
        act_energy.cumulative_value + read_energy.cumulative_value +
        write_energy.cumulative_value + ref_energy.cumulative_value +
        refb_energy.cumulative_value + Stats2DCumuSum(act_stb_energy) +
        Stats2DCumuSum(pre_stb_energy) + Stats2DCumuSum(pre_pd_energy) +
        Stats2DCumuSum(sref_energy);
    double last_queue_usage = static_cast<double>(queue_usage.epoch_value);
    double avg_queue_usage =
        (last_queue_usage * static_cast<double>(last_clk_) +
         queue_usage.epoch_value) /
        static_cast<double>(clk);
    queue_usage.cumulative_value = avg_queue_usage;
    average_power.cumulative_value = total_energy.cumulative_value / clk;
    average_bandwidth.cumulative_value =
        (reqs_issued * config_.request_size_bytes) / ((clk)*config_.tCK);

    average_latency.cumulative_value = access_latency.GetAverage();
    average_interarrival.cumulative_value = interarrival_latency.GetAverage();
}

void Statistics::PrintStats(std::ostream& where) const {
    for (auto stat : stats_list) {
        stat->Print(where);
    }
    for (auto stat : histo_stats_list) {
        stat->Print(where);
    }
}

void Statistics::UpdateEpoch(uint64_t clk) {
    // get clk information so that we can calculate power, bandwidth, etc.
    for (auto stat : stats_list) {
        stat->UpdateEpoch();
    }
    for (auto stat : histo_stats_list) {
        stat->UpdateEpoch();
    }
    last_clk_ = clk;
    return;
}

void Statistics::PrintEpochStats(std::ostream& where) const {
    for (auto stat : stats_list) {
        stat->PrintEpoch(where);
    }
    return;
}

void Statistics::PrintStatsCSVHeader(std::ostream& where) const {
    for (auto stat : stats_list) {
        stat->PrintCSVHeader(where);
    }
    for (auto stat : histo_stats_list) {
        stat->PrintCSVHeader(where);
    }
    where << std::endl;
    return;
}

void Statistics::PrintStatsCSVFormat(std::ostream& where) const {
    for (auto stat : stats_list) {
        stat->PrintCSVFormat(where);
    }
    for (auto stat : histo_stats_list) {
        stat->PrintCSVFormat(where);
    }
    where << std::endl;
    return;
}

void Statistics::PrintEpochStatsCSVFormat(std::ostream& where) const {
    for (auto stat : stats_list) {
        stat->PrintEpochCSVFormat(where);
    }
    where << std::endl;
    return;
}

double Statistics::Stats2DEpochSum(
    const std::vector<std::vector<DoubleComputeStat>>& stats_vector) {
    double stats_sum = 0.0;
    for (unsigned i = 0; i < stats_vector.size(); i++) {
        for (unsigned j = 0; j < stats_vector[i].size(); j++) {
            stats_sum += stats_vector[i][j].epoch_value;
        }
    }
    return stats_sum;
}

double Statistics::Stats2DCumuSum(
    const std::vector<std::vector<DoubleComputeStat>>& stats_vector) {
    double stats_sum = 0.0;
    for (unsigned i = 0; i < stats_vector.size(); i++) {
        for (unsigned j = 0; j < stats_vector[i].size(); j++) {
            stats_sum += stats_vector[i][j].cumulative_value;
        }
    }
    return stats_sum;
}
template <class T>
void Statistics::Push2DStatsToList(std::vector<std::vector<T>>& stats_vector) {
    for (unsigned i = 0; i < stats_vector.size(); i++) {
        for (unsigned j = 0; j < stats_vector[i].size(); j++) {
            stats_list.push_back(&(stats_vector[i][j]));
        }
    }
}

template <class T>
void Statistics::Init2DStats(std::vector<std::vector<T>>& stats_vector,
                             int shape_x, int shape_y, std::string x_string,
                             std::string y_string, std::string stat_name,
                             std::string stat_desc) {
    for (int i = 0; i < shape_x; i++) {
        std::vector<T> x_stats;
        for (int j = 0; j < shape_y; j++) {
            std::string short_desc =
                stat_name + "_" + std::to_string(i) + "_" + std::to_string(j);
            std::string long_desc = stat_desc + x_string + std::to_string(i) +
                                    y_string + std::to_string(j);
            T stat = T(short_desc, long_desc);
            x_stats.push_back(stat);
        }
        stats_vector.push_back(x_stats);
    }
}

void Statistics::PrintEpochHistoStatsCSVFormat(std::ostream& where) const {
    if (last_clk_ == 0) {
        where << "name,value,count,epoch" << std::endl;
    }
    for (auto stat : histo_stats_list) {
        stat->PrintEpochCSVFormat(where);
    }
    return;
}

std::ostream& operator<<(std::ostream& os, Statistics& stats) {
    stats.PrintStats(os);
    return os;
}

}  // namespace dramsim3
