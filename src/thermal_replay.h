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
    ThermalReplay::ThermalReplay(std::string trace_name, std::string config_file, int repeat);
    ~ThermalReplay();
    void Run();
private:
    ThermalCalculator *thermal_calc_;    
    std::string trace_name_;
    std::ifstream trace_file_;
    int repeat_;
    void ParseLine(std::string line, uint64_t &clk, Command &cmd);
};

}


#endif