#include <iostream>
#include "memory_system.h"
#include "hmc.h"
#include "cpu.h"
#include "./../ext/headers/args.hxx"

using namespace std;
using namespace dramcore;

int main(int argc, const char **argv)
{
    args::ArgumentParser parser("This is a test program.", "This goes after the options.");
    args::ValueFlag<uint64_t > numb_cycles_arg(parser, "numb_cycles", "Number of cycles to simulate", {'n', "numb-cycles"});
    args::ValueFlag<std::string> config_arg(parser, "config", "The config file", {'c', "config-file"});
    args::Flag enable_trace_cpu_arg(parser, "trace cpu", "Enable trace cpu", {"trace-cpu"});
    args::Flag is_hmc_arg(parser, "HMC system", "Set this flag ONLY if it is an HMC", {"hmc"});
    args::ValueFlag<std::string> trace_file_arg(parser, "trace", "The trace file", {"trace-file"});
    parser.ParseCLI(argc, argv);

    uint64_t cycles = args::get(numb_cycles_arg);
    bool enable_trace_cpu = enable_trace_cpu_arg;
    bool is_hmc = is_hmc_arg;
    std::string config_file, trace_file;
    config_file = args::get(config_arg);
    trace_file = args::get(trace_file_arg);

    BaseMemorySystem *memory_system;
    if (is_hmc) {
        memory_system = new HMCSystem(config_file, callback_func);
    } else {
        memory_system = new MemorySystem(config_file, callback_func);
    }

    CPU* cpu;
    if (enable_trace_cpu)
        cpu = new TraceBasedCPU(*memory_system, trace_file);
    else
        cpu = new RandomCPU(*memory_system);

    for(uint64_t clk = 0; clk < cycles; clk++) {
        cpu->ClockTick();
        memory_system->ClockTick();
    }

    memory_system->PrintStats();

    return 0;
}

