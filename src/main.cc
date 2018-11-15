#include <iostream>
#include "./../ext/headers/args.hxx"
#include "cpu.h"

// only exception of namespace as this will never collide with other namespaces
using namespace dramsim3;

int main(int argc, const char **argv) {
    args::ArgumentParser parser("DRAM Simulator.", "");
    args::HelpFlag help(parser, "help", "Display the help menu", {"h", "help"});
    args::ValueFlag<uint64_t> numb_cycles_arg(parser, "numb_cycles",
                                              "Number of cycles to simulate",
                                              {'n', "numb-cycles"}, 100000);
    args::ValueFlag<std::string> config_arg(parser, "config", "The config file",
                                            {'c', "config-file"});
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output-dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::ValueFlag<std::string> cpu_arg(parser, "cpu-type",
                                         "Type of cpu - random, trace, stream",
                                         {"cpu-type"}, "random");
    args::ValueFlag<std::string> trace_file_arg(
        parser, "trace", "The trace file", {"trace-file"});

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    uint64_t cycles = args::get(numb_cycles_arg);
    std::string config_file, output_dir, trace_file, cpu_type;
    config_file = args::get(config_arg);
    output_dir = args::get(output_dir_arg);
    trace_file = args::get(trace_file_arg);
    cpu_type = args::get(cpu_arg);

    CPU *cpu;
    if (cpu_type == "random") {
        cpu = new RandomCPU(config_file, output_dir);
    } else if (cpu_type == "trace") {
        cpu = new TraceBasedCPU(config_file, output_dir, trace_file);
    } else if (cpu_type == "stream") {
        cpu = new StreamCPU(config_file, output_dir);
    } else {
        cpu = nullptr;
        std::cerr << "Unknown cpu type" << std::endl;
        AbruptExit(__FILE__, __LINE__);
    }

    for (uint64_t clk = 0; clk < cycles; clk++) {
        cpu->ClockTick();
    }

    delete (cpu);

    return 0;
}
