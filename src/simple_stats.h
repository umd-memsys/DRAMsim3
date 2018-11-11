#ifndef __SIMPLE_STATS_
#define __SIMPLE_STATS_

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "configuration.h"

namespace dramsim3 {

class SimpleStats {
   public:
    SimpleStats(const Config& config, int channel_id);
    // incrementing counter
    void Increment(const std::string name) { counters_[name] += 1; }

    // incrementing for vec counter
    void IncrementVec(const std::string name, int pos) {
        vec_counters_[name][pos] += 1;
    }

    // increment vec counter by number
    void IncrementVecBy(const std::string name, int pos, int num) {
        vec_counters_[name][pos] += num;
    }

    // add historgram value
    void AddValue(const std::string name, const int value);

    // Print CSV stuff: same format for epoch and final outputs
    void PrintCSVHeader(std::ostream& csv_output) const;
    void PrintCSVRow(std::ostream& csv_output) const;

    // Epoch update
    void PrintEpochStats(uint64_t clk, std::ostream& csv_output,
                         std::ostream& histo_output);

    // Final statas output
    void PrintFinalStats(uint64_t clk, std::ostream& txt_output,
                         std::ostream& csv_output, std::ostream& hist_output);

    // Return rank background energy for thermal calculation
    double RankBackgroundEnergy(const int rank) const;

   private:
    void InitStat(std::string name, std::string stat_type,
                  std::string description);
    void InitVecStat(std::string name, std::string stat_type,
                     std::string description, std::string part_name,
                     int vec_len);
    void InitHistoStat(std::string name, std::string description, int start_val,
                       int end_val, int num_bins);

    uint64_t CounterEpoch(const std::string name) {
        return counters_[name] - last_counters_[name];
    }

    uint64_t VecCounterEpoch(const std::string name, const int i) {
        return vec_counters_[name][i] - last_vec_counters_[name][i];
    }

    void UpdateHistoBins();
    double GetHistoAvg(const std::string name) const;
    double GetHistoEpochAvg(const std::string name) const;
    std::string GetTextHeader(bool is_final) const;
    void UpdateEpochStats();
    void UpdateFinalStats();

    const Config& config_;
    int channel_id_;

    // map names to descriptions
    std::unordered_map<std::string, std::string> header_descs_;

    std::vector<std::pair<std::string, std::string> > print_pairs_;

    // counter stats, indexed by their name
    std::vector<std::string> counter_names_;
    std::unordered_map<std::string, uint64_t> counters_;
    std::unordered_map<std::string, uint64_t> last_counters_;

    // vectored counter stats, first indexed by name then by index
    std::vector<std::string> vec_counter_names_;
    std::unordered_map<std::string, std::vector<uint64_t> > vec_counters_;
    std::unordered_map<std::string, std::vector<uint64_t> > last_vec_counters_;

    // NOTE: doubles_ vec_doubles_ and calculated_ are basically one time
    // placeholders after each epoch they store the value for that epoch
    // (different from the counters) and in the end updated to the overall value
    std::vector<std::string> double_names_;
    std::unordered_map<std::string, double> doubles_;

    std::vector<std::string> vec_double_names_;
    std::unordered_map<std::string, std::vector<double> > vec_doubles_;

    // calculated stats, similar to double, but not the same
    std::vector<std::string> calculated_names_;
    std::unordered_map<std::string, double> calculated_;

    // histogram stats
    std::vector<std::string> histo_names_;
    std::unordered_map<std::string, std::vector<std::string> > histo_headers_;

    std::unordered_map<std::string, std::pair<int, int> > histo_bounds_;
    std::unordered_map<std::string, int> bin_widths_;
    std::unordered_map<std::string, std::unordered_map<int, uint64_t> >
        histo_counts_;
    std::unordered_map<std::string, std::unordered_map<int, uint64_t> >
        last_histo_counts_;
    std::unordered_map<std::string, std::vector<uint64_t> > histo_bins_;
    std::unordered_map<std::string, std::vector<uint64_t> > last_histo_bins_;
};

}  // namespace dramsim3
#endif