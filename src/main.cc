#include <iostream>
#include <cstdlib>
#include <fstream>
#include "controller.h"
#include "timing.h"
#include "common.h"
#include "config.h"
#include "cpu.h"

using namespace std;

int main(int argc, char **argv)
{
    bool enable_trace_cpu = false;
    Config config;
    Timing timing(config);

    vector<Controller*> ctrls(config.channels);
    for(auto i = 0; i < config.channels; i++) {
        ctrls[i] = new Controller(i, config, timing);
    }

    TraceBasedCPU trace_cpu(ctrls, config);
    RandomCPU random_cpu(ctrls, config);

    for(auto clk = 0; clk < config.cycles; clk++) {
        enable_trace_cpu ? trace_cpu.ClockTick() : random_cpu.ClockTick();
        for( auto ctrl : ctrls)
            ctrl->ClockTick();
    }
    return 0;
}