#ifndef __SIMPLE_STATS_
#define __SIMPLE_STATS_

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "configuration.h"
#include "json.hpp"

namespace dramsim3 {

class SimpleStats {
   public:
    SimpleStats(const Config& config, int channel_id);
    // incrementing counter
    void Increment(const std::string name) { epoch_counters_[name] += 1; tag_counters_[name] += 1;}

    void SetTag(uint64_t tag) {epoch_counters_["tag"] = tag; tag_counters_["tag"] = tag;}

    // incrementing for vec counter
    void IncrementVec(const std::string name, int pos) {
        epoch_vec_counters_[name][pos] += 1;
        tag_vec_counters_[name][pos] += 1;
    }

    // increment vec counter by number
    void IncrementVecBy(const std::string name, int pos, int num) {
        epoch_vec_counters_[name][pos] += num;
        tag_vec_counters_[name][pos] += num;
    }

    // add historgram value
    void AddValue(const std::string name, const int value);

    // Epoch update
    void PrintEpochStats();

    void PrintTagStats();

    // Final statas output
    void PrintFinalStats();

    // Reset (usually after one phase of simulation)
    void Reset();

   private:
    using VecStat = std::unordered_map<std::string, std::vector<uint64_t> >;
    using HistoCount = std::unordered_map<int, uint64_t>;
    using Json = nlohmann::json;
    enum CounterSet {FINAL = 0, EPOCH, TAG};
    void InitStat(std::string name, std::string stat_type,
                  std::string description);
    void InitVecStat(std::string name, std::string stat_type,
                     std::string description, std::string part_name,
                     int vec_len);
    void InitHistoStat(std::string name, std::string description, int start_val,
                       int end_val, int num_bins);

    void UpdateCounters();
    void UpdateHistoBins();
    void UpdateTagHistoBins();
    void UpdatePrints(CounterSet set);
    double GetHistoAvg(const HistoCount& histo_counts) const;
    std::string GetTextHeader(bool is_final) const;
    void UpdateEpochStats();
    void UpdateTagStats();
    void UpdateFinalStats();

    const Config& config_;
    int channel_id_;

    // map names to descriptions
    std::unordered_map<std::string, std::string> header_descs_;


    //There are two sets of Counter (Epoch and Tag) and
    //Printing Functions (PrintEpochStats and PrintTagStats)
    //within each class
    //memory_system->dram_system->controller->simple_stats

    //Within each set there are three types of
    //{set}_counters_ {set}_vec_counters {set}_histo_counts_
    //Variavles without {set} are used for final tally and are
    //accumlated from Epoch counters

    //The only difference between the two sets of Counters is
    //the period when they print to a file and reset their counters

    //Epoch - prints every "epoch_period" cycle which is set in each
    //dramsim3 config file after it prints a period, the stats are
    //accumulated in the total counter and then reset to 0

    //Tag - prints whenever "print_stat_v_i" in manycore_tb_top is
    //set high, along with the associated tag in "print_stat_tag_i".
    //This is the same with how vanilla_stats.csv gets logged.
    //After it prints the counter is reset to 0, but they are not
    //accumulated.

    // counter stats, indexed by their name
    std::unordered_map<std::string, uint64_t> counters_;
    std::unordered_map<std::string, uint64_t> epoch_counters_;
    std::unordered_map<std::string, uint64_t> tag_counters_;

    // vectored counter stats, first indexed by name then by index
    VecStat vec_counters_;
    VecStat epoch_vec_counters_;
    VecStat tag_vec_counters_;


    // NOTE: doubles_ vec_doubles_ and calculated_ are basically one time
    // placeholders after each epoch they store the value for that epoch
    // (different from the counters) and in the end updated to the overall value
    std::unordered_map<std::string, double> doubles_;

    std::unordered_map<std::string, std::vector<double> > vec_doubles_;

    // calculated stats, similar to double, but not the same
    std::unordered_map<std::string, double> calculated_;

    // histogram stats
    std::unordered_map<std::string, std::vector<std::string> > histo_headers_;

    std::unordered_map<std::string, std::pair<int, int> > histo_bounds_;
    std::unordered_map<std::string, int> bin_widths_;
    std::unordered_map<std::string, HistoCount> histo_counts_;
    std::unordered_map<std::string, HistoCount> epoch_histo_counts_;
    std::unordered_map<std::string, HistoCount> tag_histo_counts_;
    VecStat histo_bins_;
    VecStat epoch_histo_bins_;
    VecStat tag_histo_bins_;


    // outputs
    Json j_data_;
    std::vector<std::pair<std::string, std::string> > print_pairs_;
};

}  // namespace dramsim3
#endif
