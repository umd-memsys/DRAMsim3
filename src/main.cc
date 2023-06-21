#include <iostream>
#include <iomanip> 
#include <locale>
#include "./../ext/headers/args.hxx"
#include "cpu.h"
#include "custom_cpu.h"


using namespace dramsim3;

int main(int argc, const char **argv) {
    #ifdef MY_DEBUG
    std::cout<<"== "<<__func__<<" == ";
    std::cout<<" main "<<std::endl;
    #endif
    args::ArgumentParser parser(
        "DRAM Simulator.",
        "Examples: \n."
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100 -t "
        "sample_trace.txt\n"
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -s random -c 100\n"
        "./build/dramsim3main configs/DDR4_8Gb_x4_3200_LRDIMM.ini -g random -c 100");
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::ValueFlag<uint64_t> num_cycles_arg(parser, "num_cycles",
                                             "Number of cycles to simulate",
                                             {'c', "cycles"}, 100000);
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output_dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::ValueFlag<std::string> stream_arg(
        parser, "stream_type", "address stream generator - (random), stream",
        {'s', "stream"}, "");
    args::ValueFlag<std::string> custom_gen_arg(
        parser, "generator_type", "transaction generator, random(RANDOM), stream (STREAM)",
        {'g', "generator"}, "");
    args::ValueFlag<std::string> kernel_type_arg(
        parser, "kernel_type", "NDP kernel typess - EWA, EWM,..",
        {'k', "kernel"}, "");        
    args::ValueFlag<std::string> trace_file_arg(
        parser, "trace",
        "Trace file, setting this option will ignore -s option",
        {'t', "trace"});
    args::Positional<std::string> config_arg(
        parser, "config", "The config file name (mandatory)");

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

    std::string config_file = args::get(config_arg);
    if (config_file.empty()) {
        std::cerr << parser;
        return 1;
    }

    uint64_t cycles = args::get(num_cycles_arg);
    std::string output_dir = args::get(output_dir_arg);
    std::string trace_file = args::get(trace_file_arg);
    std::string stream_type = args::get(stream_arg);
    std::string generator_type = args::get(custom_gen_arg);
    std::string kernel_type = args::get(kernel_type_arg);

    if(generator_type != "")  {  
        CUSTOM_CPU *cpu = new CUSTOM_CPU(config_file, output_dir, generator_type);
        if(generator_type == "kernel" || generator_type =="KERNEL") {
            /*                    
                Generate memory requests for NDP kernel exeuction 
                Simulation Steps
                    Before Start Simulation 
                        - Generatation Reference Data 
                        - Generation NDP Instruction
                        - Generation NDP Data
                    Start Simulation 
                        - Preloading NDP Data 
                        - Preloading NDP Instruction
                        - NDP Kernel Execution 
                        - Read Result
                    End Simulation
                        - check the result
            */

           std::cout<<"================================"<<std::endl;   
           std::cout<<"Simulation Prepare ... "<<std::endl;
           std::cout<<"================================"<<std::endl;
           // Generatation NDP Configuration Request
           cpu->genNDPConfig(kernel_type);

           // Generatation Reference Data 
           cpu->genRefData(kernel_type);

           // Generation NDP Instruction 
           cpu->genNDPInst(kernel_type);

           // Generation NDP Data
           cpu->genNDPData(kernel_type);

           // Generation NDP Execution
           cpu->genNDPExec(kernel_type);           

           // Generate NDP Result Read Request
           cpu->genNDPReadResult(kernel_type);           

           std::cout<<"================================"<<std::endl;   
           std::cout<<"Simulation Start ... "<<std::endl;
           std::cout<<"================================"<<std::endl;
           // Simulation Start
           while(1) {
               cpu->ClockTick();
               if(cpu->simDone()) break;
           } 
           std::cout<<"================================"<<std::endl;
           std::cout<<"Simulation Done ... "<<std::endl;   
           std::cout<<"================================"<<std::endl;
           // Check NDP Result with Reference Data
           cpu->checkNDPResult(kernel_type);

        }
        else {
            // Generate memory requests randomly or sequentially
            for (uint64_t clk = 0; clk < cycles; clk++) {
                cpu->ClockTick();
            }
        }

        // Check Result
        cpu->PrintStats();

        // delete CPU object
        cpu->printResult();
        delete cpu;        
    } 
    else {
        CPU *cpu;
        if (!trace_file.empty()) {
            cpu = new TraceBasedCPU(config_file, output_dir, trace_file);
        } else {
            if (stream_type == "stream" || stream_type == "s") {
                cpu = new StreamCPU(config_file, output_dir);
            } else {
                cpu = new RandomCPU(config_file, output_dir);
            }
        }

        for (uint64_t clk = 0; clk < cycles; clk++) {
            cpu->ClockTick();
        }
        cpu->PrintStats();

        delete cpu;
    }

    return 0;
}
