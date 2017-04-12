#include <iostream>
#include "controller.h"
#include "cpu.h"
#include "memory_system.h"

using namespace std;
using namespace dramcore;

void callback_func(uint64_t req_id); //TODO - Avoid Forward declaration of the dummy callback function

int main(int argc, char **argv)
{
    bool enable_trace_cpu = true;

    MemorySystem memory_system(callback_func);

    TraceBasedCPU trace_cpu(memory_system, memory_system.config_);
    RandomCPU random_cpu(memory_system, memory_system.config_);

    for(auto clk = 0; clk < memory_system.config_.cycles; clk++) {
        enable_trace_cpu ? trace_cpu.ClockTick() : random_cpu.ClockTick();
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