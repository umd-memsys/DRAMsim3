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
    : config_(config), channel_id_(channel_id) {
    // counter stats
    InitStat("num_cycles", "counter", "Number of DRAM cycles");
    InitStat("num_epochs", "counter", "Number of epochs");
    InitStat("num_reads_done", "counter", "Number of read requests issued");
    InitStat("num_writes_done", "counter", "Number of read requests issued");
    InitStat("num_write_buf_hits", "counter", "Number of write buffer hits");
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

    // Histogram stats
    InitHistoStat("read_latency", "Read request latency in cycles", 0, 200, 10);
    InitHistoStat("interarrival_latency", "Request interarrival latency", 0,
                  100, 10);

    // some irregular stats
    InitStat("average_bandwidth", "calculated", "Average bandwidth");
    InitStat("total_energy", "calculated", "Total energy (pJ)");
    InitStat("average_power", "calculated", "Average power (mW)");
    InitStat("average_read_latency", "calculated",
             "Average read request latency");
    InitStat("average_interarrival", "calculated",
             "Average request interarrival latency");
}

void SimpleStats::AddValue(const std::string name, const int value) {
    auto& counts = histo_counts_[name];
    if (counts.count(value) <= 0) {
        counts[value] = 1;
    } else {
        counts[value] += 1;
    }
}

void SimpleStats::PrintCSVHeader(std::ostream& csv_output) const {
    // a little hacky but should work as long as iterating channel 0 first
    if (channel_id_ != 0) {
        return;
    }
    csv_output << "channel,";
    for (const auto& it : print_pairs_) {
        csv_output << it.first << ",";
    }
    csv_output << std::endl;
}

std::string SimpleStats::GetTextHeader(bool is_final) const {
    std::string header =
        "###########################################\n## Statistics of "
        "Channel " +
        std::to_string(channel_id_);
    if (!is_final) {
        header += " of epoch " + std::to_string(counters_.at("num_epochs"));
    }
    header += "\n###########################################\n";
    return header;
}

void SimpleStats::PrintCSVRow(std::ostream& csv_output) const {
    csv_output << channel_id_ << ",";
    for (const auto& it : print_pairs_) {
        csv_output << it.second << ",";
    }
    csv_output << std::endl;
}

void SimpleStats::PrintEpochStats(uint64_t clk, std::ostream& csv_output) {
    UpdateEpochStats();
    if (config_.output_level >= 0) {
        std::cout << GetTextHeader(false);
        for (const auto& it : print_pairs_) {
            PrintStatText(std::cout, it.first, it.second,
                          header_descs_[it.first]);
        }
    }
    if (config_.output_level >= 1) {
        if (counters_["num_epochs"] == 1) {
            PrintCSVHeader(csv_output);
        }
        PrintCSVRow(csv_output);
    }
    print_pairs_.clear();
}

void SimpleStats::PrintFinalStats(uint64_t clk, std::ostream& txt_output,
                                  std::ostream& csv_output,
                                  std::ostream& hist_output) {
    UpdateFinalStats();
    if (config_.output_level >= 0) {
        std::cout << GetTextHeader(true);
        for (const auto& it : print_pairs_) {
            PrintStatText(std::cout, it.first, it.second,
                          header_descs_[it.first]);
        }
    }

    if (config_.output_level >= 1) {
        txt_output << GetTextHeader(true);
        for (const auto& it : print_pairs_) {
            PrintStatText(txt_output, it.first, it.second,
                          header_descs_[it.first]);
        }
        PrintCSVHeader(csv_output);
        PrintCSVRow(csv_output);
    }

    // print detailed histogram breakdown
    if (config_.output_level >= 2) {
        // header
        if (channel_id_ == 0) {
            hist_output << "channel, name, value, count" << std::endl;
        }
        //will be out of order but you already need some lib to process it
        for (const auto& name : histo_names_) {
            for (auto it : histo_counts_[name]) {
                hist_output << channel_id_ << "," << name << "," << it.first
                            << "," << it.second << std::endl;
            }
        }
    }
}

double SimpleStats::RankBackgroundEnergy(const int rank) const {
    return vec_doubles_.at("act_stb_energy")[rank] +
           vec_doubles_.at("pre_stb_energy")[rank] +
           vec_doubles_.at("sref_energy")[rank];
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
    }
}

void SimpleStats::InitHistoStat(std::string name, std::string description,
                                int start_val, int end_val, int num_bins) {
    histo_names_.push_back(name);
    int bin_width = (end_val - start_val) / num_bins;
    bin_widths_.emplace(name, bin_width);
    histo_bounds_.emplace(name, std::make_pair(start_val, end_val));
    histo_counts_.emplace(name, std::unordered_map<int, uint64_t>());
    last_histo_counts_.emplace(name, std::unordered_map<int, uint64_t>());

    // initialize headers, descriptions
    std::vector<std::string> headers;
    auto header = fmt::format("{}[-{}]", name, start_val);
    headers.push_back(header);
    header_descs_.emplace(header, description);
    for (int i = 1; i < num_bins + 1; i++) {
        int bucket_start = start_val + (i - 1) * bin_width;
        int bucket_end = start_val + i * bin_width - 1;
        header = fmt::format("{}[{}-{}]", name, bucket_start, bucket_end);
        headers.push_back(header);
        header_descs_.emplace(header, description);
    }
    header = fmt::format("{}[{}-]", name, end_val);
    headers.push_back(header);
    header_descs_.emplace(header, description);

    histo_headers_.emplace(name, headers);

    // +2 for front and end
    histo_bins_.emplace(name, std::vector<uint64_t>(num_bins + 2, 0));
    last_histo_bins_.emplace(name, std::vector<uint64_t>(num_bins + 2, 0));
}

void SimpleStats::UpdateHistoBins() {
    for (const auto& name : histo_names_) {
        auto& bins = histo_bins_[name];
        std::fill(bins.begin(), bins.end(), 0);
        for (const auto it : histo_counts_[name]) {
            int value = it.first;
            uint64_t count = it.second;
            int bin_idx = 0;
            if (value < histo_bounds_[name].first) {
                bin_idx = 0;
            } else if (value > histo_bounds_[name].second) {
                bin_idx = bins.size() - 1;
            } else {
                bin_idx =
                    (value - histo_bounds_[name].first) / bin_widths_[name] + 1;
            }
            bins[bin_idx] += count;
        }
    }
}

double SimpleStats::GetHistoAvg(const std::string name) const {
    const auto& counts = histo_counts_.at(name);
    uint64_t accu_sum = 0;
    uint64_t count = 0;
    for (auto i = counts.begin(); i != counts.end(); i++) {
        accu_sum += i->first * i->second;
        count += i->second;
    }
    return static_cast<double>(accu_sum) / static_cast<double>(count);
}

double SimpleStats::GetHistoEpochAvg(const std::string name) const {
    const auto& counts = histo_counts_.at(name);
    uint64_t accu_sum = 0;
    uint64_t count = 0;
    for (auto i = counts.begin(); i != counts.end(); i++) {
        accu_sum += i->first * i->second;
        count += i->second;
    }
    const auto& last_counts = last_histo_counts_.at(name);
    uint64_t last_sum = 0;
    uint64_t last_count = 0;
    for (auto i = last_counts.begin(); i != last_counts.end(); i++) {
        last_sum += i->first * i->second;
        last_count += i->second;
    }
    return static_cast<double>(accu_sum - last_sum) /
           static_cast<double>(count - last_count);
}

void SimpleStats::UpdateEpochStats() {
    // push counter values as is
    for (const auto& name : counter_names_) {
        print_pairs_.emplace_back(name, std::to_string(CounterEpoch(name)));
    }

    // update computed stats
    doubles_["act_energy"] =
        CounterEpoch("num_act_cmds") * config_.act_energy_inc;
    doubles_["read_energy"] =
        CounterEpoch("num_read_cmds") * config_.read_energy_inc;
    doubles_["write_energy"] =
        CounterEpoch("num_write_cmds") * config_.write_energy_inc;
    doubles_["ref_energy"] =
        CounterEpoch("num_ref_cmds") * config_.ref_energy_inc;
    doubles_["refb_energy"] =
        CounterEpoch("num_refb_cmds") * config_.refb_energy_inc;
    for (const auto& name : double_names_) {
        print_pairs_.emplace_back(name, fmt::format("{}", doubles_[name]));
    }

    // vector counters
    for (const auto& name : vec_counter_names_) {
        for (size_t i = 0; i < vec_counters_[name].size(); i++) {
            std::string trailing = "." + std::to_string(i);
            std::string print_name = name + trailing;
            print_pairs_.emplace_back(print_name,
                                      std::to_string(VecCounterEpoch(name, i)));
        }
    }

    // vector doubles, update first, then push
    double background_energy = 0.0;
    for (int i = 0; i < config_.ranks; i++) {
        double act_stb = VecCounterEpoch("rank_active_cycles", i) *
                         config_.act_stb_energy_inc;
        double pre_stb = VecCounterEpoch("all_bank_idle_cycles", i) *
                         config_.pre_stb_energy_inc;
        double sref_energy =
            VecCounterEpoch("sref_cycles", i) * config_.sref_energy_inc;
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

    UpdateHistoBins();
    for (const auto& name : histo_names_) {
        const auto& headers = histo_headers_[name];
        const auto& bins = histo_bins_[name];
        const auto& last_bins = last_histo_bins_[name];
        for (size_t i = 0; i < bins.size(); i++) {
            auto epoch_count = bins[i] - last_bins[i];
            print_pairs_.emplace_back(headers[i], std::to_string(epoch_count));
        }
    }

    // calculated stats
    uint64_t total_reqs =
        CounterEpoch("num_reads_done") + CounterEpoch("num_writes_done");
    double total_time = CounterEpoch("num_cycles") * config_.tCK;
    double avg_bw = total_reqs * config_.request_size_bytes / total_time;
    calculated_["average_bandwidth"] = avg_bw;

    double total_energy = doubles_["act_energy"] + doubles_["read_energy"] +
                          doubles_["write_energy"] + doubles_["ref_energy"] +
                          doubles_["refb_energy"] + background_energy;
    calculated_["total_energy"] = total_energy;
    calculated_["average_power"] = total_energy / CounterEpoch("num_cycles");
    calculated_["average_read_latency"] = GetHistoEpochAvg("read_latency");
    calculated_["average_interarrival"] =
        GetHistoEpochAvg("interarrival_latency");

    for (const auto& name : calculated_names_) {
        print_pairs_.emplace_back(name, fmt::format("{}", calculated_[name]));
    }

    // finally update last epoch values
    for (const auto& name : counter_names_) {
        last_counters_[name] = counters_[name];
    }
    last_counters_["num_epochs"] = 0;  // NOTE: this is an exception
    for (const auto& name : vec_counter_names_) {
        last_vec_counters_[name] = vec_counters_[name];
    }
    for (const auto& name : histo_names_) {
        last_histo_counts_[name] = histo_counts_[name];
        last_histo_bins_[name] = histo_bins_[name];
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
    doubles_["read_energy"] =
        counters_["num_read_cmds"] * config_.read_energy_inc;
    doubles_["write_energy"] =
        counters_["num_write_cmds"] * config_.write_energy_inc;
    doubles_["ref_energy"] = counters_["num_ref_cmds"] * config_.ref_energy_inc;
    doubles_["refb_energy"] =
        counters_["num_refb_cmds"] * config_.refb_energy_inc;
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
        double act_stb =
            vec_counters_["rank_active_cycles"][i] * config_.act_stb_energy_inc;
        double pre_stb = vec_counters_["all_bank_idle_cycles"][i] *
                         config_.pre_stb_energy_inc;
        double sref_energy =
            vec_counters_["sref_cycles"][i] * config_.sref_energy_inc;
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

    // histograms
    UpdateHistoBins();
    for (const auto& name : histo_names_) {
        const auto& headers = histo_headers_[name];
        const auto& bins = histo_bins_[name];
        for (size_t i = 0; i < bins.size(); i++) {
            print_pairs_.emplace_back(headers[i], std::to_string(bins[i]));
        }
    }

    // calculated stats
    uint64_t total_reqs =
        counters_["num_reads_done"] + counters_["num_writes_done"];
    double total_time = counters_["num_cycles"] * config_.tCK;
    double avg_bw = total_reqs * config_.request_size_bytes / total_time;
    calculated_["average_bandwidth"] = avg_bw;

    double total_energy = doubles_["act_energy"] + doubles_["read_energy"] +
                          doubles_["write_energy"] + doubles_["ref_energy"] +
                          doubles_["refb_energy"] + background_energy;
    calculated_["total_energy"] = total_energy;
    calculated_["average_power"] = total_energy / counters_["num_cycles"];
    calculated_["average_read_latency"] = GetHistoAvg("read_latency");
    calculated_["average_interarrival"] = GetHistoAvg("interarrival_latency");

    for (const auto& name : calculated_names_) {
        print_pairs_.emplace_back(name, fmt::format("{}", calculated_[name]));
    }
    return;
}

}  // namespace dramsim3
