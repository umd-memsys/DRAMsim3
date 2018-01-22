#include "thermal_replay.h"
#include "./../ext/headers/args.hxx"

using namespace dramcore;

ThermalReplay::ThermalReplay(std::string trace_name, std::string config_file, uint64_t repeat):
    repeat_(repeat)
{
    trace_file_.open(trace_name);
    if (!trace_file_) {
        std::cout << "cannot open trace file " << trace_name << std::endl;
        exit(1);
    }

    Config config(config_file);
    Statistics stats(config);
    thermal_calc_ = new ThermalCalculator(config, stats);
}

ThermalReplay::~ThermalReplay() {
    delete(thermal_calc_);
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
            cout << "epoch " << i << " " << clk + clk_offset << " " << cmd << endl;
            // TODO need to update stats before final calculation
            thermal_calc_->UpdatePower(cmd, clk + clk_offset);
        }
        trace_file_.clear();
        trace_file_.seekg(0);
        clk_offset += clk;
        thermal_calc_->PrintTransPT(clk);
    }
    thermal_calc_->PrintFinalPT(clk_offset);
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


