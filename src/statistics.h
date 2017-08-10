#ifndef __STATISTICS_H
#define __STATISTICS_H

#include <iostream>
#include <string>
#include <list>
#include <vector>
#include "configuration.h"

namespace dramcore {


class BaseStat {
public:
    BaseStat() {}
    BaseStat(std::string name, std::string description) { name_ = name; description_ = description; }
    virtual void Print(std::ostream& where) const = 0;
    virtual void UpdateEpoch() = 0;
    virtual void PrintEpoch(std::ostream& where) const = 0;
    virtual void PrintCSVHeader(std::ostream& where) const = 0;
    virtual void PrintCSVFormat(std::ostream& where) const = 0;
    virtual void PrintEpochCSVFormat(std::ostream& where) const = 0;
    friend std::ostream& operator<<(std::ostream& os, const BaseStat& basestat) { basestat.Print(os); return os;}
protected:
    std::string name_;
    std::string description_;
};

class CounterStat : public BaseStat{
public:
    CounterStat():BaseStat() {}
    CounterStat(std::string name, std::string desc);
    void operator=(int count) { count_ = count; }
    uint64_t Count() { return count_; }
    uint64_t LastCount() { return last_epoch_count_; }
    CounterStat& operator++() { count_++; return *this; }
    CounterStat& operator++(int) { count_++; return *this; }
    CounterStat& operator--() { count_--; return *this; }
    CounterStat& operator--(int) { count_--; return *this; }
    void Print(std::ostream& where) const override ;
    void UpdateEpoch() override ;
    void PrintEpoch(std::ostream& where) const override ;
    void PrintCSVHeader(std::ostream& where) const override ;
    void PrintCSVFormat(std::ostream& where) const override ;
    void PrintEpochCSVFormat(std::ostream& where) const override ;
private:
    uint64_t count_;
    uint64_t last_epoch_count_;
};


class HistogramStat : public BaseStat {
public:
    HistogramStat():BaseStat() {}
    HistogramStat(int start, int end, uint32_t numb_bins, std::string name, std::string desc);
    void AddValue(int val);
    void Print(std::ostream& where) const override ;
    void UpdateEpoch() override ;
    void PrintEpoch(std::ostream& where) const override ;
    void PrintCSVHeader(std::ostream& where) const override ;
    void PrintCSVFormat(std::ostream& where) const override ;
    void PrintEpochCSVFormat(std::ostream& where) const override ;
private:
    int start_;
    int end_;
    uint32_t numb_bins_;
    std::vector<uint64_t> bin_count_, last_epoch_bin_count_;
    uint64_t neg_outlier_count_, pos_outlier_count_, last_epoch_neg_outlier_count_, last_epoch_pos_outlier_count_;
};


class DoubleStat : public BaseStat {
public:
    DoubleStat():BaseStat() {};
    DoubleStat(double inc, std::string name, std::string desc);
    void operator=(double new_value) {value = new_value; }
    DoubleStat& operator++(int) {value += inc_; return *this; }
    void Print(std::ostream& where) const override ;
    void UpdateEpoch() override ;
    void PrintEpoch(std::ostream& where) const override ;
    void PrintCSVHeader(std::ostream& where) const override ;
    void PrintCSVFormat(std::ostream& where) const override ;
    void PrintEpochCSVFormat(std::ostream& where) const override ;
    double value;
    double last_epoch_value;
private:
    double inc_;
};


class NonCumulativeStat : public BaseStat {
    // each epoch only print the value within this epoch 
    // and so is the final print
public:
    NonCumulativeStat() : BaseStat() {};
    NonCumulativeStat(std::string name, std::string desc);
    void Print(std::ostream& where) const override ;
    void UpdateEpoch() override ;
    void PrintEpoch(std::ostream& where) const override ;
    void PrintCSVHeader(std::ostream& where) const override ;
    void PrintCSVFormat(std::ostream& where) const override ;
    void PrintEpochCSVFormat(std::ostream& where) const override ;
    double value;
};


class Statistics {
public:
    Statistics(const Config& config);
    class CounterStat numb_read_reqs_issued;
    class CounterStat numb_write_reqs_issued;
    class CounterStat hmc_reqs_done;
    class CounterStat numb_row_hits;
    class CounterStat numb_read_row_hits;
    class CounterStat numb_write_row_hits;
    class CounterStat numb_aggressive_precharges;
    class CounterStat numb_ondemand_precharges;
    class CounterStat dramcycles;

    class HistogramStat access_latency;
    class CounterStat numb_buffered_requests;

    class CounterStat hbm_dual_command_issue_cycles;
    class CounterStat hbm_dual_non_rw_cmd_attempt_cycles;

    class CounterStat numb_read_cmds_issued;
    class CounterStat numb_write_cmds_issued;
    class CounterStat numb_activate_cmds_issued;
    class CounterStat numb_precharge_cmds_issued;
    class CounterStat numb_refresh_cmds_issued;
    class CounterStat numb_refresh_bank_cmds_issued;
    class CounterStat numb_self_refresh_enter_cmds_issued;
    class CounterStat numb_self_refresh_exit_cmds_issued;

    class CounterStat numb_rw_rowhits_pending_refresh;

#ifdef DEBUG_POWER
    class CounterStat active_cycles;
    class CounterStat all_bank_idle_cycles;
#endif // DEBUG_POWER
    
    // energy and power stats
    class DoubleStat act_energy;
    class DoubleStat read_energy;
    class DoubleStat write_energy;
    class DoubleStat ref_energy;    // rank ref
    class DoubleStat refb_energy;  // bank refresh
    std::vector<std::vector<DoubleStat>> act_stb_energy;  // active standby
    std::vector<std::vector<DoubleStat>> pre_stb_energy;  // precharge standby
    std::vector<std::vector<DoubleStat>> pre_pd_energy;  // precharge powerdown energy
    std::vector<std::vector<DoubleStat>> sref_energy;  // self ref energy
    class DoubleStat total_energy;
    class NonCumulativeStat average_power;

    class NonCumulativeStat average_bandwidth;
    
    std::list<class BaseStat*> stats_list;

    void PrintStats(std::ostream& where) const;
    void UpdateEpoch(uint64_t clk);
    void UpdatePreEpoch(uint64_t clk);
    void PrintEpochStats(std::ostream& where) const;
    void PrintStatsCSVHeader(std::ostream& where) const;
    void PrintStatsCSVFormat(std::ostream& where) const;
    void PrintEpochStatsCSVFormat(std::ostream& where) const;

    friend std::ostream& operator<<(std::ostream& os, Statistics& stats);

    static double Sum(const std::vector<std::vector<DoubleStat>>& stats_vector);
    template<class T>
    void PushStatsVecToList(std::vector<std::vector<T>>& stats_vector);
    void InitStatsPerRank(std::vector<std::vector<DoubleStat>>& stats_vector,
                          double inc_value, std::string stat_name, std::string stat_desc);
private:
    const Config& config_;
    uint64_t last_clk_;
};

}
#endif