#include <iostream>
#include "memory_system.h"
#include "hmc.h"
#include "cpu.h"
#include "./../ext/headers/args.hxx"

using namespace std;
using namespace dramcore;

int main(int argc, const char **argv)
{
    args::ArgumentParser parser("DRAM Simulator.", "");
    args::HelpFlag help(parser, "help", "Display the help menu", {"h", "help"});
    args::ValueFlag<uint64_t > numb_cycles_arg(parser, "numb_cycles", "Number of cycles to simulate", {'n', "numb-cycles"}, 100000);
    args::ValueFlag<std::string> config_arg(parser, "config", "The config file", {'c', "config-file"});
    args::ValueFlag<std::string> output_dir_arg(parser, "output-dir", "Output directory for stats files", {'o', "output-dir"}, ".");
    args::ValueFlag<std::string> cpu_arg(parser, "cpu-type", "Type of cpu - random, trace, stream", {"cpu-type"}, "random");
    args::ValueFlag<std::string> memory_type_arg(parser, "memory_type", "Type of memory system - default, hmc, ideal", {"memory-type"}, "default");
    args::ValueFlag<std::string> trace_file_arg(parser, "trace", "The trace file", {"trace-file"});

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

    uint64_t cycles = args::get(numb_cycles_arg);
    std::string config_file, output_dir, trace_file, cpu_type, memory_system_type;
    config_file = args::get(config_arg);
    output_dir = args::get(output_dir_arg);
    trace_file = args::get(trace_file_arg);
    cpu_type = args::get(cpu_arg);
    memory_system_type = args::get(memory_type_arg);

    BaseMemorySystem *memory_system;
    if(memory_system_type == "default") { //TODO - Better name than default?
        memory_system = new MemorySystem(config_file, output_dir, read_callback_func, write_callback_func);
    }
    else if(memory_system_type == "hmc") {
        memory_system = new HMCMemorySystem(config_file, output_dir, read_callback_func, write_callback_func);
    }
    else if(memory_system_type == "ideal") {
        memory_system = new IdealMemorySystem(config_file, output_dir, read_callback_func, write_callback_func);
    }
    else {
        cout << "Unknown memory system type" << endl;
        AbruptExit(__FILE__, __LINE__);
    }

    CPU* cpu;
    if(cpu_type == "random") {
        cpu = new RandomCPU(*memory_system);
    }
    else if(cpu_type == "trace") {
        cpu = new TraceBasedCPU(*memory_system, trace_file);
    }
    else if(cpu_type == "stream") {
        cpu = new StreamCPU(*memory_system);
    }
    else {
        cout << "Unknown cpu type" << endl;
        AbruptExit(__FILE__, __LINE__);
    }
        
    for(uint64_t clk = 0; clk < cycles; clk++) {
        cpu->ClockTick();
        memory_system->ClockTick();
    }

    memory_system->PrintStats();

    return 0;
}

