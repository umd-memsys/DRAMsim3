#include "simple_stats.h"
#include "../ext/fmt/src/format.h"

namespace dramsim3 {

template <class T>
void PrintStatText(std::ostream& where, std::string name, T value,
                   std::string description) {
    // not making this a class method because we need to calculate
    // power & bw later, which are not BaseStat members
    where << fmt::format("{:<30}{:^3}{:>12}{:>5}{}", name, " = ", value, " # ",
                         description)
          << std::endl;
    return;
}

SimpleStats::SimpleStats(const Config& config, int channel_id)
    : config_(config), last_clk_(0), channel_id_(channel_id) {
    InitStat("num_cycles", "counter", "Number of DRAM cycles");
    InitStat("num_epochs", "counter", "Number of epochs");
    InitStat("num_reads_done", "counter", "Number of read requests issued");
    InitStat("num_writes_done", "counter", "Number of read requests issued");
    InitStat("read_energy", "double", "Read energy");
    InitStat("average_bandwidth", "double", "Average bandwidth");
    InitVecStat("all_bank_idle_cycles", "vec_counter",
                "Cyles of all bank idle in rank", "rank", config_.ranks);
}

void SimpleStats::Increment(const std::string name) { counters_[name] += 1; }

void SimpleStats::Increment(const std::string name, int pos) {
    vec_counters_[name][pos] += 1;
}

void SimpleStats::PrintEpochStats(uint64_t clk, std::ostream& csv_output,
                                  std::ostream& histo_output) {
    UpdateStats(true);
    std::cout << "Channel " << channel_id_ << " Epoch Stats!" << std::endl;
    for (const auto& it : print_pairs_) {
        PrintStatText(std::cout, it.first, it.second, header_descs_[it.first]);
    }
    print_pairs_.clear();
}

void SimpleStats::PrintStats(uint64_t clk, std::ostream& txt_output,
                             std::ostream& csv_output,
                             std::ostream& hist_output) {
    UpdateStats(false);
    for (const auto& name : counter_names_) {
        PrintStatText(std::cout, name, counters_[name], header_descs_[name]);
    }
    for (const auto& name : double_names_) {
        PrintStatText(std::cout, name, doubles_[name], header_descs_[name]);
    }
}

void SimpleStats::InitStat(std::string name, std::string stat_type,
                           std::string description) {
    header_descs_.emplace(name, description);
    if (stat_type == "counter") {
        counter_names_.push_back(name);
        counters_.emplace(name, 0);
        last_counters_.emplace(name, 0);
    } else if (stat_type == "double") {
        double_names_.push_back(name);
        doubles_.emplace(name, 0);
        last_doubles_.emplace(name, 0);
    }
}

void SimpleStats::InitVecStat(std::string name, std::string stat_type,
                              std::string description, std::string part_name,
                              int vec_len) {
    for (int i = 0; i < vec_len; i++) {
        std::string trailing = "." + std::to_string(i);
        std::string actual_name = name + trailing;
        std::string actual_desc = description + " " + part_name + trailing;
        header_descs_.emplace(actual_name, actual_desc);
    }
    if (stat_type == "vec_counter") {
        vec_counter_names_.push_back(name);
        vec_counters_.emplace(name, std::vector<uint64_t>(vec_len, 0));
        last_vec_counters_.emplace(name, std::vector<uint64_t>(vec_len, 0));
    } else if (stat_type == "vec_double") {
        vec_double_names_.push_back(name);
        vec_doubles_.emplace(name, std::vector<double>(vec_len, 0));
        last_vec_doubles_.emplace(name, std::vector<double>(vec_len, 0));
    }
}

void SimpleStats::UpdateStats(bool epoch) {
    // first calculate the epoch values to be printed at the time
    for (const auto& name : counter_names_) {
        uint64_t epoch_val = CounterEpoch(name);
        print_pairs_.emplace_back(name, std::to_string(epoch_val));
    }

    // update computed stats
    double read_energy_epoch =
        CounterEpoch("num_reads_done") * config_.read_energy_inc;
    doubles_["read_energy"] = last_doubles_["read_energy"] + read_energy_epoch;
    print_pairs_.emplace_back("read_energy", std::to_string(read_energy_epoch));

    // these things behave differently based on whether it's an epoch or final
    uint64_t total_reqs, total_clks;
    if (epoch) {
        total_reqs =
            CounterEpoch("num_reads_done") + CounterEpoch("num_writes_done");
        total_clks = CounterEpoch("num_cycles");
    } else {
        total_reqs = counters_["num_reads_done"] + counters_["num_writes_done"];
        total_clks = counters_["num_cycles"];
    }
    double avg_bw =
        total_reqs * config_.request_size_bytes / (total_clks * config_.tCK);
    doubles_["average_bandwidth"] = avg_bw;
    print_pairs_.emplace_back("average_bandwidth", std::to_string(avg_bw));

    // update vector counters
    for (const auto& name : vec_counter_names_) {
        auto& vec = vec_counters_[name];
        auto& last_vec = last_vec_counters_[name];
        for (size_t i = 0; i < vec.size(); i++) {
            std::string trailing = "." + std::to_string(i);
            uint64_t epoch_count = vec[i] - last_vec[i];
            std::string print_name = name + trailing;
            print_pairs_.emplace_back(print_name, std::to_string(epoch_count));
        }
    }

    // finally update last epoch values
    for (const auto& name : counter_names_) {
        last_counters_[name] = counters_[name];
    }
    last_counters_["num_epochs"] = 0;  // this is an exception to other counters
    for (const auto& name : double_names_) {
        last_doubles_[name] = doubles_[name];
    }
    for (const auto& name : vec_counter_names_) {
        last_vec_counters_[name] = vec_counters_[name];
    }
    for (const auto& name : vec_double_names_) {
        last_vec_doubles_[name] = vec_doubles_[name];
    }
    return;
}

}  // namespace dramsim3
