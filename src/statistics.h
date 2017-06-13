#ifndef __STATISTICS_H
#define __STATISTICS_H

#include <iostream>
#include <string>
#include <list>
#include <vector>

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
    CounterStat& operator++() { count_++; return *this; }
    CounterStat& operator++(int) { count_++; return *this; }
    CounterStat& operator--() { count_--; return *this; }
    CounterStat& operator--(int) { count_--; return *this; }
    void Print(std::ostream& where) const;
    void UpdateEpoch();
    void PrintEpoch(std::ostream& where) const;
    void PrintCSVHeader(std::ostream& where) const;
    void PrintCSVFormat(std::ostream& where) const;
    void PrintEpochCSVFormat(std::ostream& where) const;
private:
    uint64_t count_;
    uint64_t last_epoch_count_;
};


class HistogramStat : public BaseStat {
public:
    HistogramStat():BaseStat() {}
    HistogramStat(int start, int end, uint32_t numb_bins, std::string name, std::string desc);
    void AddValue(int val);
    void Print(std::ostream& where) const;
    void UpdateEpoch();
    void PrintEpoch(std::ostream& where) const;
    void PrintCSVHeader(std::ostream& where) const;
    void PrintCSVFormat(std::ostream& where) const;
    void PrintEpochCSVFormat(std::ostream& where) const;
private:
    int start_;
    int end_;
    uint32_t numb_bins_;
    std::vector<uint64_t> bin_count_, last_epoch_bin_count_;
    uint64_t neg_outlier_count_, pos_outlier_count_, last_epoch_neg_outlier_count_, last_epoch_pos_outlier_count_;
};

class Statistics {
public:
    Statistics();
    class CounterStat numb_read_reqs_issued;
    class CounterStat numb_write_reqs_issued;
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

    class CounterStat numb_rw_rowhits_pending_refresh;

    std::list<class BaseStat*> stats_list;

    void PrintStats(std::ostream& where) const;
    void UpdateEpoch();
    void PrintEpochStats(std::ostream& where) const;
    void PrintStatsCSVHeader(std::ostream& where) const;
    void PrintStatsCSVFormat(std::ostream& where) const;
    void PrintEpochStatsCSVFormat(std::ostream& where) const;

    friend std::ostream& operator<<(std::ostream& os, Statistics& stats);
};

}
#endif