[![Build Status](https://travis-ci.com/shavvn/DRAMSim3.svg?token=pCfCJ4yBqyhn3rfWbJVF&branch=master)](https://travis-ci.com/shavvn/DRAMSim3)

# About DRAMSim3

	DRAMSim3 models the timing paramaters and memory controller behavior for several DRAM protocols such as DDR3, 
    DDR4, LPDDR3, LPDDR4, GDDR5, GDDR6, HBM, HMC, STT-MRAM. It is implemented in C++ as an objected oriented model 
    that includes a parameterized DRAM bank model, DRAM controllers, command queues and system-level interfaces to 
    interact with a CPU simulator (GEM5, ZSim) or trace workloads. It is designed to be accurate, portable and parallel.

    For more information on new features, along with hierarchical design, please read:
    Shang Li, Zhiyuan Yang, Dhiraj Reddy, Ankur Srivastava, and Bruce Jacob. 2019.
    DRAMsim3: A Cycle-accurate, thermal capable memory system simulator. IEEE
    Computer Architecture Letters (2019).
    (I didn't fink public link for that one)

    [\[2\] Li et al. *Rethinking Cycle Accurate DRAM Simulation* ACM MEMSYS'19.]
    (https://user.eng.umd.edu/~blj/papers/memsys2019-rethinking.pdf) 


## Building and running the simulator

This simulator by default uses a CMake based build system.
The advantage in using a CMake based build system is portability and dependency management.
We require CMake 3.0+ to build this simulator.
If `cmake-3.0` is not available,
we also supply a Makefile to build the most basic version of the simulator.

### Building

Doing out of source builds with CMake is recommended to avoid the build files cluttering the main directory.

```bash
# cmake out of source build
mkdir build
cd build
cmake ..

# Build dramsim3 library and executables
make -j4

# Alternatively, build with thermal module enabled
cmake .. -DTHERMAL=1

```

The build process creates `dramsim3main` and executables in the `build` directory.
By default, it also creates `libdramsim3.so` shared library in the project root directory.

### Running

```bash
# help
./build/dramsim3main -h

# Running random stream with a config file
./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini --stream random -c 100000 

# Running a trace file
./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100000 -t sample_trace.txt

# Running with gem5
--mem-type=dramsim3 --dramsim3-ini=configs/DDR4_4Gb_x4_2133.ini

```

The output can be directed to another directory by `-o` option
or can be configured in the config file.
You can control the verbosity in the config file as well.

### Output Visualization

`scripts/plot_stats.py` can visualize some of the output (requires `matplotlib`):

```bash
# generate histograms from overall output
python3 scripts/plot_stats dramsim3.json

# or
# generate time series for a variety stats from epoch outputs
python3 scripts/plot_stats dramsim3epoch.json
```

Currently stats from all channels are squashed together for cleaner plotting.

## Simulator Design

### Code Structure
├── configs                 # Configs of various protocols that describe timing constraints and power consumption.
├── ext                     # 
├── scripts                 # Tools and utilities
├── src                     # DRAMSim3 source files
├── tests                   # Tests of each model, includes a short example trace
├── CMakeLists.txt
├── Makefile
├── LICENSE
└── README.md

├── src  
    bankstate.cc: Records and manages DRAM bank timings and states which is modeled as a state machine.
    channelstate.cc: Records and manages channel timings and states.
    command_queue.cc: Maintains per-bank or per-rank FIFO queueing structures, determine which commands in the queues can be issued in this cycle.
    configuration.cc: Initiates, manages system and DRAM parameters, including protocol, DRAM timings, address mapping policy and power parameters.
    controller.cc: Maintains the per-channel controller, which manages a queue of pending memory transactions and issues corresponding DRAM commands, 
                   follows FR-FCFS policy.
    cpu.cc: Implements 3 types of simple CPU: 
            1. Random, can handle random CPU requests at full speed, the entire parallelism of DRAM protocol can be exploited without limits from 
            address mapping and scheduling pocilies. 
            2. Stream, provides a streaming prototype that is able to provide enough buffer hits.
            3. Trace-based, consumes traces of workloads, feed the fetched transactions into the memory system.
    dram_system.cc:  Initiates JEDEC or ideal DRAM system, registers the supplied callback function to let the front end driver know that the request is               finished. 
    hmc.cc: Implements HMC system and interface, HMC requests are translates to DRAM requests here and a crossbar interconnect between the high-speed links and the memory controllers is modeled.
    main.cc: Handles the main program loop that reads in simulation arguments, DRAM configurations and tick cycle forward.
    memory_system.cc: A wrapper of dram_system and hmc.
    refresh.cc: Raises refresh request based on per-rank refresh or per-bank refresh.
    timing.cc: Initiate timing constraints.

## Experiments

### Verilog Validation

First we generate a DRAM command trace.
There is a `CMD_TRACE` macro and by default it's disabled.
Use `cmake .. -DCMD_TRACE=1` to enable the command trace output build and then
whenever a simulation is performed the command trace file will be generated.

Next, `scripts/validation.py` helps generate a Verilog workbench for Micron's Verilog model
from the command trace file.
Currently DDR3, DDR4, and LPDDR configs are supported by this script.

Run

```bash
./script/validataion.py DDR4.ini cmd.trace
```

To generage Verilog workbench.
Our workbench format is compatible with ModelSim Verilog simulator,
other Verilog simulators may require a slightly different format.
