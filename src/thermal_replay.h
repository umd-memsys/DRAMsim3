#ifndef __THERMAL_REPLAY_H
#define __THERMAL_REPLAY_H

#include <string>
#include <fstream>

#include "common.h"
#include "configuration.h"
#include "thermal.h"
#include "statistics.h"

namespace dramcore {

class ThermalReplay {
public:
    ThermalReplay(std::string trace_name, std::string config_file, uint64_t repeat);
    ~ThermalReplay();
    void Run();
private:
    std::ifstream trace_file_;
    Config config_;
    Statistics stats_;
    ThermalCalculator thermal_calc_;    
    uint64_t repeat_;
    uint64_t last_clk_;
    std::vector<std::vector<std::vector<std::vector<bool>>>> bank_active_;
    void ParseLine(std::string line, uint64_t &clk, Command &cmd);
    void ProcessCMD(Command &cmd, uint64_t clk);
    bool IsRankActive(int channel, int rank);
};

}


#endif