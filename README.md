[![Build Status](https://travis-ci.com/shavvn/DRAMSim3.svg?token=pCfCJ4yBqyhn3rfWbJVF&branch=master)](https://travis-ci.com/shavvn/DRAMSim3)

# DRAMSim3
Model the timing paramaters and memory controller behaviour for several DRAM protocols such as DDR3, DDR4, LPDDR3, LPDDR4, GDDR5, HBM, HMC, STT-MRAM.


## Building and running the simulator
This simulator by default uses a CMake based build system.
The advantage in using a CMake based build system is portability and dependency management.
We require CMake 3.0+ to build this simulator.
If `cmake-3.0` is not available,
we also supply a Makefile to build the most basic version of the simulator.

## From the command line

### Building
Doing out of source builds with CMake is recommended to avoid the build files cluttering the main directory.

```bash
# Create and jump to the build directory
mkdir build 
cd build

# Create Makefile using the CMakeLists.txt file in the parent directory
cmake ..

# Alternatively, build with thermal module enabled (still testing)
cmake .. -DTHERMAL=1

# Build dramsim3 and executables
make -j4

```

The build process creates `dramsim3main` and executables in the build 
directory. 
By default, it also creates `libdramsim3.so` shared library in the 
project root directory.


### Running

```bash
# help
./build/dramsim3main -h

# Running random stream with a config file
./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini --stream random -c 100000 

# Running trace cpu with a trace file and a config file
./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100000 -t sample_trace.txt

```

The output can be directed to another directory by `-o` option
or can be configured in the config file. 
You can control the verbosity in the config file as well.

### Plotting (in dev)

`scripts/plot_stats.py` can visualize some of the output (requires `matplotlib`):

```bash
# generate histograms from overall output
python3 scripts/plot_stats dramsim.json

# or
# generate time series for a variety stats from epoch outputs
python3 scripts/plot_stats dramsim_epoch.json
```

Currently stats from all channels are squashed together for cleaner plotting.
