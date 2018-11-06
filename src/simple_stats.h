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
    void Increment(const std::string name);

    // incrementing for vec counter
    void IncrementVec(const std::string name, int pos);

    // Epoch update
    void PrintEpochStats(uint64_t clk, std::ostream& csv_output,
                         std::ostream& histo_output);

    // Final statas output
    void PrintFinalStats(uint64_t clk, std::ostream& txt_output,
                    std::ostream& csv_output, std::ostream& hist_output);

   private:
    uint64_t CounterEpoch(const std::string name) {
        return counters_[name] - last_counters_[name];
    }

    void InitStat(std::string name, std::string stat_type,
                  std::string description);
    void InitVecStat(std::string name, std::string stat_type,
                     std::string description, std::string part_name,
                     int vec_len);

    void UpdateEpochStats();
    void UpdateFinalStats();

    const Config& config_;
    uint64_t last_clk_;
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

    // double type counterparts
    std::vector<std::string> double_names_;
    std::unordered_map<std::string, double> doubles_;
    std::unordered_map<std::string, double> last_doubles_;

    std::vector<std::string> vec_double_names_;
    std::unordered_map<std::string, std::vector<double> > vec_doubles_;
    std::unordered_map<std::string, std::vector<double> > last_vec_doubles_;

    // calculated stats, similar to double, but not the same
    std::vector<std::string> calculated_names_;
    std::unordered_map<std::string, double> calculated_;
};

}  // namespace dramsim3
#endif