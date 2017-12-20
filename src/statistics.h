#ifndef __STATISTICS_H
#define __STATISTICS_H

#include <iostream>
#include <string>
#include <list>
#include <map>
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
    double CountDouble() {return static_cast<double>(count_); }
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
    void PrintEpoch(std::ostream& where) const {} // don't print histogram, put them in csv instead
    void PrintCSVHeader(std::ostream& where) const override ;
    void PrintCSVFormat(std::ostream& where) const override ;
    void PrintEpochCSVFormat(std::ostream& where) const override ;
    std::vector<uint64_t> GetAggregatedBins() const;
    double GetAverage() const;
    uint64_t AccuSum() const;  // Accumulated Sum (value * count)
    uint64_t CountSum() const;
private:
    int start_;
    int end_;
    uint32_t numb_bins_;
    std::map<int, uint64_t> bins_;
    std::map<int, uint64_t> last_epoch_bins_;
    uint64_t epoch_count_;
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


class DoubleComputeStat : public BaseStat {
public:
    DoubleComputeStat() : BaseStat() {};
    DoubleComputeStat(std::string name, std::string desc);
    void Print(std::ostream& where) const override ;
    void UpdateEpoch() override ;
    void PrintEpoch(std::ostream& where) const override ;
    void PrintCSVHeader(std::ostream& where) const override ;
    void PrintCSVFormat(std::ostream& where) const override ;
    void PrintEpochCSVFormat(std::ostream& where) const override ;
    double epoch_value;
    double cumulative_value;
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

    std::vector<std::vector<CounterStat>> sref_cycles;
    std::vector<std::vector<CounterStat>> active_cycles;
    std::vector<std::vector<CounterStat>> all_bank_idle_cycles;

    // energy and power stats
    
    class DoubleComputeStat act_energy;
    class DoubleComputeStat read_energy;
    class DoubleComputeStat write_energy;
    class DoubleComputeStat ref_energy;
    class DoubleComputeStat refb_energy;
    std::vector<std::vector<DoubleComputeStat>> act_stb_energy;  // active standby
    std::vector<std::vector<DoubleComputeStat>> pre_stb_energy;  // active standby
    std::vector<std::vector<DoubleComputeStat>> pre_pd_energy;  // active standby
    std::vector<std::vector<DoubleComputeStat>> sref_energy;  // active standby

    // class DoubleComputeStat act_stb_energy;
    // class DoubleComputeStat pre_stb_energy;
    // class DoubleComputeStat pre_pd_energy;
    // class DoubleComputeStat sref_energy;
    class DoubleComputeStat total_energy;
    class DoubleComputeStat queue_usage;
    class DoubleComputeStat average_power;
    class DoubleComputeStat average_bandwidth;
    class DoubleComputeStat average_latency;
    class DoubleComputeStat average_interarrival;
   
    // histogram stats
    class HistogramStat access_latency;
    class HistogramStat interarrival_latency;

    std::list<class BaseStat*> stats_list;
    std::list<class HistogramStat*> histo_stats_list;

    void PrintStats(std::ostream& where) const;
    void UpdateEpoch(uint64_t clk);
    void PreEpochCompute(uint64_t clk);
    void PrintEpochStats(std::ostream& where) const;
    void PrintStatsCSVHeader(std::ostream& where) const;
    void PrintStatsCSVFormat(std::ostream& where) const;
    void PrintEpochStatsCSVFormat(std::ostream& where) const;
    void PrintEpochHistoStatsCSVFormat(std::ostream& where) const;

    friend std::ostream& operator<<(std::ostream& os, Statistics& stats);

    static double Stats2DEpochSum(const std::vector<std::vector<DoubleComputeStat>>& stats_vector);
    static double Stats2DCumuSum(const std::vector<std::vector<DoubleComputeStat>>& stats_vector);
    template<class T>
    void Push2DStatsToList(std::vector<std::vector<T>>& stats_vector);
    template<class T>
    void Init2DStats(std::vector<std::vector<T>> &stats_vector, int shape_x, int shape_y, std::string x_string, std::string y_string, std::string stat_name, std::string stat_desc);
private:
    const Config& config_;
    uint64_t last_clk_;
};

}
#endif