#include <iostream>
#include "controller.h"
#include "cpu.h"
#include "statistics.h"

using namespace std;

int main(int argc, char **argv)
{
    bool enable_trace_cpu = true;
    Config config;
    Timing timing(config);
    Statistics stats;

    vector<Controller*> ctrls(config.channels);
    for(auto i = 0; i < config.channels; i++) {
        ctrls[i] = new Controller(i, config, timing, stats);
    }

    TraceBasedCPU trace_cpu(ctrls, config);
    RandomCPU random_cpu(ctrls, config);

    for(auto clk = 0; clk < config.cycles; clk++) {
        enable_trace_cpu ? trace_cpu.ClockTick() : random_cpu.ClockTick();
        for( auto ctrl : ctrls)
            ctrl->ClockTick();
    }

    cout << "-----------------------------------------------------" << endl;
    cout << "Printing Statistics -- " << endl;
    cout << "-----------------------------------------------------" << endl;
    cout << stats;
    return 0;
}