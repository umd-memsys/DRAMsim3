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
    // counter stats
    InitStat("num_cycles", "counter", "Number of DRAM cycles");
    InitStat("num_epochs", "counter", "Number of epochs");
    InitStat("num_reads_done", "counter", "Number of read requests issued");
    InitStat("num_writes_done", "counter", "Number of read requests issued");
    InitStat("num_write_buf_hits", "counter", "Number of write buffer hits");
    InitStat("num_row_hits", "counter", "Number of row buffer hits");
    InitStat("num_read_row_hits", "counter", "Number of read row buffer hits");
    InitStat("num_write_row_hits", "counter",
             "Number of write row buffer hits");
    InitStat("num_read_cmds", "counter", "Number of READ/READP commands");
    InitStat("num_write_cmds", "counter", "Number of WRITE/WRITEP commands");
    InitStat("num_act_cmds", "counter", "Number of ACT commands");
    InitStat("num_pre_cmds", "counter", "Number of PRE commands");
    InitStat("num_ondemand_pres", "counter", "Number of ondemend PRE commands");
    InitStat("num_ref_cmds", "counter", "Number of REF commands");
    InitStat("num_refb_cmds", "counter", "Number of REFb commands");
    InitStat("num_srefe_cmds", "counter", "Number of SREFE commands");
    InitStat("num_srefx_cmds", "counter", "Number of SREFX commands");
    InitStat("hbm_dual_cmds", "counter", "Number of cycles dual cmds issued");

    // double stats
    InitStat("act_energy", "double", "Activation energy");
    InitStat("read_energy", "double", "Read energy");
    InitStat("write_energy", "double", "Write energy");
    InitStat("ref_energy", "double", "Refresh energy");
    InitStat("refb_energy", "double", "Refresh-bank energy");

    // some irregular stats
    InitStat("average_bandwidth", "calculated", "Average bandwidth");
    InitStat("total_energy", "calculated", "Total energy (pJ)");
    InitStat("average_read_latency", "calculated",
             "Average read request latency");
    InitStat("average_interarrival", "calculated",
             "Average request interarrival latency");

    // Vector counter stats
    InitVecStat("all_bank_idle_cycles", "vec_counter",
                "Cyles of all bank idle in rank", "rank", config_.ranks);
    InitVecStat("rank_active_cycles", "vec_counter", "Cyles of rank active",
                "rank", config_.ranks);
    InitVecStat("sref_cycles", "vec_counter", "Cyles of rank in SREF mode",
                "rank", config_.ranks);

    // Vector of double stats
    InitVecStat("act_stb_energy", "vec_double", "Active standby energy", "rank",
                config_.ranks);
    InitVecStat("pre_stb_energy", "vec_double", "Precharge standby energy",
                "rank", config_.ranks);
    InitVecStat("sref_energy", "vec_double", "SREF energy", "rank",
                config_.ranks);
}

void SimpleStats::Increment(const std::string name) { counters_[name] += 1; }

void SimpleStats::IncrementVec(const std::string name, int pos) {
    vec_counters_[name][pos] += 1;
}

void SimpleStats::PrintEpochStats(uint64_t clk, std::ostream& csv_output,
                                  std::ostream& histo_output) {
    UpdateEpochStats();
    std::cout << "Channel " << channel_id_ << " Epoch Stats!" << std::endl;
    for (const auto& it : print_pairs_) {
        PrintStatText(std::cout, it.first, it.second, header_descs_[it.first]);
    }
    print_pairs_.clear();
}

void SimpleStats::PrintFinalStats(uint64_t clk, std::ostream& txt_output,
                                  std::ostream& csv_output,
                                  std::ostream& hist_output) {
    UpdateFinalStats();
    for (const auto& it : print_pairs_) {
        PrintStatText(std::cout, it.first, it.second, header_descs_[it.first]);
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
        doubles_.emplace(name, 0.0);
        last_doubles_.emplace(name, 0);
    } else if (stat_type == "calculated") {
        calculated_names_.push_back(name);
        calculated_.emplace(name, 0.0);
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

void SimpleStats::UpdateEpochStats() {
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
    uint64_t total_reqs =
        CounterEpoch("num_reads_done") + CounterEpoch("num_writes_done");
    double total_time = CounterEpoch("num_cycles") * config_.tCK;
    double avg_bw = total_reqs * config_.request_size_bytes / total_time;
    calculated_["average_bandwidth"] = avg_bw;
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
    last_counters_["num_epochs"] = 0;  // NOTE: this is an exception
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

void SimpleStats::UpdateFinalStats() {
    // push counter values as is
    for (const auto& name : counter_names_) {
        print_pairs_.emplace_back(name, std::to_string(counters_[name]));
    }

    // update computed stats
    doubles_["act_energy"] = counters_["num_act_cmds"] * config_.act_energy_inc;
    doubles_["read_energy"] = counters_["num_read_cmds"] * config_.read_energy_inc;
    doubles_["write_energy"] = counters_["num_write_cmds"] * config_.write_energy_inc;
    doubles_["ref_energy"] = counters_["num_ref_cmds"] * config_.ref_energy_inc;
    doubles_["refb_energy"] = counters_["num_refb_cmds"] * config_.refb_energy_inc;
    for (const auto& name : double_names_) {
        print_pairs_.emplace_back(name, fmt::format("{}", doubles_[name]));
    }

    // vector counters
    for (const auto& name : vec_counter_names_) {
        const auto& vec = vec_counters_[name];
        for (size_t i = 0; i < vec.size(); i++) {
            std::string trailing = "." + std::to_string(i);
            std::string print_name = name + trailing;
            print_pairs_.emplace_back(print_name, std::to_string(vec[i]));
        }
    }

    // vector doubles, update first, then push
    double background_energy = 0.0;
    for (int i = 0; i < config_.ranks; i++) {
        double act_stb = vec_counters_["rank_active_cycles"][i] * config_.act_stb_energy_inc;
        double pre_stb = vec_counters_["all_bank_idle_cycles"][i] * config_.pre_stb_energy_inc;
        double sref_energy = vec_counters_["sref_cycles"][i] * config_.sref_energy_inc;
        vec_doubles_["act_stb_energy"][i] = act_stb;
        vec_doubles_["pre_stb_energy"][i] = pre_stb;
        vec_doubles_["sref_energy"][i] = sref_energy;
        background_energy += act_stb + pre_stb + sref_energy;
    }
    for (const auto& name : vec_double_names_) {
        const auto& vec = vec_doubles_[name];
        for (size_t i = 0; i < vec.size(); i++) {
            std::string trailing = "." + std::to_string(i);
            std::string print_name = name + trailing;
            print_pairs_.emplace_back(print_name, fmt::format("{}", vec[i]));
        }
    }

    // calculated stats
    uint64_t total_reqs =
        counters_["num_reads_done"] + counters_["num_writes_done"];
    double total_time = counters_["num_cycles"] * config_.tCK;
    double avg_bw = total_reqs * config_.request_size_bytes / total_time;
    calculated_["average_bandwidth"] = avg_bw;

    double total_energy = doubles_["act_energy"] + doubles_["read_energy"] +
        doubles_["write_energy"] + doubles_["ref_energy"] + doubles_["refb_energy"] + background_energy;
    // print_pairs_.emplace_back("average_bandwidth", std::to_string(avg_bw));
    calculated_["total_energy"] = total_energy;

    for (const auto& name : calculated_names_) {
        print_pairs_.emplace_back(name, fmt::format("{}", calculated_[name]));
    }
    return;
}

}  // namespace dramsim3
