#include <iostream>
#include "memory_system.h"
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
    args::ValueFlag<std::string> trace_file_arg(parser, "trace", "The trace file", {"trace-file"});
    parser.ParseCLI(argc, argv);

    uint64_t cycles = args::get(numb_cycles_arg);
    bool enable_trace_cpu = enable_trace_cpu_arg;
    std::string config_file, trace_file;
    config_file = args::get(config_arg);
    trace_file = args::get(trace_file_arg);

    MemorySystem memory_system(config_file, callback_func);

    TraceBasedCPU* trace_cpu; //TODO - Define CPU class and use inheritance
    RandomCPU* random_cpu;
    if(enable_trace_cpu)
        trace_cpu = new TraceBasedCPU(memory_system, trace_file);
    else
        random_cpu = new RandomCPU(memory_system);

    for(uint64_t clk = 0; clk < cycles; clk++) {
        enable_trace_cpu ? trace_cpu->ClockTick() : random_cpu->ClockTick();
        memory_system.ClockTick();
    }

    memory_system.PrintStats();

    return 0;
}

