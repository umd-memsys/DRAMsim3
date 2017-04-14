#include <iostream>
#include "memory_system.h"
#include "cpu.h"
#include "./../ext/args.hxx"

using namespace std;
using namespace dramcore;

void callback_func(uint64_t req_id); //TODO - Avoid Forward declaration of the dummy callback function

int main(int argc, const char **argv)
{
    args::ArgumentParser parser("This is a test program.", "This goes after the options.");
    args::ValueFlag<std::string> config_arg(parser, "config", "The config file", {'c', "config-file"});
    args::Flag enable_trace_cpu_arg(parser, "trace cpu", "Enable trace cpu", {"trace-cpu"});
    args::ValueFlag<std::string> trace_file_arg(parser, "trace", "The trace file", {"trace-file"});
    parser.ParseCLI(argc, argv);

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

    for(auto clk = 0; clk < (*memory_system.ptr_config_).cycles; clk++) { //TODO - Yuck - 3AM coding :P
        enable_trace_cpu ? trace_cpu->ClockTick() : random_cpu->ClockTick();
        memory_system.ClockTick();
    }

    memory_system.PrintStats();

    return 0;
}

//Dummy callback function for use when the simulator is not integrated with SST or other frontend feeders
void callback_func(uint64_t req_id) {
    cout << "Request with id = " << req_id << " is returned" << endl;
    return;
}