#include "thermal_replay.h"
#include "./../ext/headers/args.hxx"

using namespace dramcore;

ThermalReplay::ThermalReplay(std::string trace_name, std::string config_file, uint64_t repeat):
    config_(config_file),
    stats_(config_),
    thermal_calc_(config_, stats_),
    repeat_(repeat),
    last_clk_(0)
{
    trace_file_.open(trace_name);
    if (!trace_file_) {
        std::cout << "cannot open trace file " << trace_name << std::endl;
        exit(1);
    }

    // Initialize bank states, for power calculation we only need to know
    // if it's active
    for (int i = 0; i < config_.channels; i++) {
        std::vector<std::vector<std::vector<bool>>> chan_vec;
        for (int j = 0; j < config_.ranks; j++) {
            std::vector<std::vector<bool>> rank_vec;
            for (int k = 0; k < config_.bankgroups; k++) {
                std::vector<bool> bank_vec(config_.banks_per_group, false);
                rank_vec.push_back(bank_vec);
            }
            chan_vec.push_back(rank_vec);
        }
        bank_active_.push_back(chan_vec);
    }
}


ThermalReplay::~ThermalReplay() {
    trace_file_.close();
}

void ThermalReplay::Run(){
    uint64_t clk_offset = 0;
    for (int i = 0; i < repeat_ ; i++) {
        std::string line;
        uint64_t clk;
        while (std::getline(trace_file_, line)) {
            Command cmd;
            ParseLine(line, clk, cmd);            
            ProcessCMD(cmd, clk);
            thermal_calc_.UpdatePower(cmd, clk + clk_offset);
        }
        trace_file_.clear();
        trace_file_.seekg(0);
        clk_offset += clk;
        thermal_calc_.PrintTransPT(clk);
    }
    thermal_calc_.PrintFinalPT(clk_offset);
}

void ThermalReplay::ParseLine(std::string line, uint64_t &clk, Command &cmd) {
    std::map<std::string, CommandType> cmd_map = {
        {"read", CommandType::READ},
        {"read_p", CommandType::READ_PRECHARGE},
        {"write", CommandType::WRITE},
        {"write_p", CommandType::WRITE_PRECHARGE},
        {"activate", CommandType::ACTIVATE},
        {"precharge", CommandType::PRECHARGE},
        {"refresh_bank",  CommandType::REFRESH_BANK},// verilog model doesn't distinguish bank/rank refresh 
        {"refresh", CommandType::REFRESH},
        {"self_refresh_enter", CommandType::SELF_REFRESH_ENTER},
        {"self_refresh_exit", CommandType::SELF_REFRESH_EXIT},
    };
    std::vector<std::string> tokens = StringSplit(line, ' ');

    // basic sanity check
    if (tokens.size() != 8) {
        std::cerr << "Check trace format!" << endl;
        AbruptExit(__FILE__, __LINE__);
    }

    // converting clock
    clk = stoull(tokens[0]);

    // converting address
    Address addr(std::stoi(tokens[2]),
                 std::stoi(tokens[3]),
                 std::stoi(tokens[4]),
                 std::stoi(tokens[5]),
                 std::stoi(tokens[6]),
                 std::stoi(tokens[7]));
    
    // reassign cmd
    cmd.addr_ = addr;
    cmd.cmd_type_ = cmd_map[tokens[1]];
    return;
}


void ThermalReplay::ProcessCMD(Command &cmd, uint64_t clk) {
    // calculate background power
    // TODO add self-ref later
    uint64_t past_clks = clk - last_clk_;
    for (int i = 0; i < config_.channels; i++) {
        for (int j = 0; j < config_.ranks; j++) {
            if (IsRankActive(i, j)) {
                stats_.active_cycles[i][j] = (stats_.active_cycles[i][j].Count() + past_clks);
            } else {
                stats_.all_bank_idle_cycles[i][j] = (stats_.all_bank_idle_cycles[i][j].Count() + past_clks);
            }
        }
    }

    // update cmd count
    switch(cmd.cmd_type_) {
        case CommandType::READ:
        case CommandType::READ_PRECHARGE:
            stats_.numb_read_cmds_issued++;
            break;
        case CommandType::WRITE:
        case CommandType::WRITE_PRECHARGE:
            stats_.numb_write_cmds_issued++;
            break;
        case CommandType::ACTIVATE:
            stats_.numb_activate_cmds_issued++;
            break;
        case CommandType::PRECHARGE:
            stats_.numb_precharge_cmds_issued++;
            break;
        case CommandType::REFRESH:
            stats_.numb_refresh_cmds_issued++;
            break;
        case CommandType::REFRESH_BANK:
            stats_.numb_refresh_bank_cmds_issued++;
            break;
        case CommandType::SELF_REFRESH_ENTER:
            stats_.numb_self_refresh_enter_cmds_issued++;
            break;
        case CommandType::SELF_REFRESH_EXIT:
            stats_.numb_self_refresh_exit_cmds_issued++;
            break;
        default:
            AbruptExit(__FILE__, __LINE__);
    }

    // update stats
    stats_.PreEpochCompute(clk);
    stats_.UpdateEpoch(clk);
    last_clk_ = clk;
    return;
}


bool ThermalReplay::IsRankActive(int channel, int rank) {
    std::vector<std::vector<bool>> &rank_active = bank_active_[channel][rank];
    for (size_t i = 0; i < rank_active.size(); i++) {
        std::vector<bool> &bg_active = rank_active[i];
        for (size_t j = 0; j < bg_active.size(); j++) {
            if (bg_active[j]) {
                return true;
            }
        }
    }
    return false;
}

int main(int argc, const char **argv) {
    args::ArgumentParser parser("Thermal Replay Module", "");
    args::HelpFlag help(parser, "help", "Display the help menu", {"h", "help"});
    args::ValueFlag<uint64_t > repeat_arg(parser, "repeats", "Number of repeats", {'r', "num-repeats"}, 10);
    args::ValueFlag<std::string> config_arg(parser, "config", "The config file", {'c', "config-file"});
    args::ValueFlag<std::string> output_dir_arg(parser, "output-dir", "Output directory for stats files", {'o', "output-dir"}, "results");
    args::ValueFlag<std::string> memory_type_arg(parser, "memory_type", "Type of memory system - default, hmc, ideal", {"memory-type"}, "default");
    args::ValueFlag<std::string> trace_file_arg(parser, "trace", "The trace file", {'t', "trace-file"});

    try {
        parser.ParseCLI(argc, argv);
    }
    catch (args::Help) {
        cout << parser;
        return 0;
    }
    catch (args::ParseError e)
    {
        cerr << e.what() << endl;
        cerr << parser;
        return 1;
    }

    uint64_t repeats = args::get(repeat_arg);
    std::string config_file, output_dir, trace_file, memory_system_type;
    config_file = args::get(config_arg);
    output_dir = args::get(output_dir_arg);
    trace_file = args::get(trace_file_arg);
    memory_system_type = args::get(memory_type_arg);

    ThermalReplay thermal_replay(trace_file, config_file, repeats);

    thermal_replay.Run();

    return 0;
}


