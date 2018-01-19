#include "thermal_replay.h"

using namespace dramcore;

ThermalReplay::ThermalReplay(std::string trace_name, std::string config_file, int repeat):
    repeat_(repeat)
{
    trace_file_.open(trace_name);
    if (!trace_file_) {
        std::cout << "cannot open trace file " << trace_name << endl;
        exit(1);
    }

    Config config(config_file);
    Statistics stats(config);
    thermal_calc_ = new ThermalCalculator(config, stats);
}

void ThermalReplay::Run(){
    for (int i = 0; i < repeat_ ; i++) {

        std::string line;
        Command cmd;
        uint64_t clk;
        while (std::getline(trace_file_, line)) {
            ParseLine(line, clk, cmd);            
            thermal_calc_->UpdatePower(cmd, clk);
        }
        trace_file_.clear();
        trace_file_.seekg(0);
    }
}

void ThermalReplay::ParseLine(std::string line, uint64_t &clk, Command &cmd) {
    Address addr;
    CommandType cmd_type;
    // TODO parse line and write back to cmd and clk
}


ThermalReplay::~ThermalReplay() {
    trace_file_.close();
}

