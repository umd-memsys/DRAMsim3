[![Build Status](https://travis-ci.com/shavvn/DRAMSim3.svg?token=pCfCJ4yBqyhn3rfWbJVF&branch=master)](https://travis-ci.com/shavvn/DRAMSim3)

# DRAMSim3
Model the timing paramaters and memory controller behaviour for several DRAM protocols such as DDR3, DDR4, LPDDR3, LPDDR4, GDDR5, HBM, HMC.


## Building and running the simulator
This simulator uses a CMake based build system. The advantage in using a CMake based build system is that the project can be readily
ported to and worked on from any IDE (atleast in theory!).

## From the command line

### Building
Doing out of source builds with CMake is recommended to avoid the build files cluttering the main directory.

```
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

```
// Running random cpu with a config file
./build/dramsim3main -c configs/DDR4_8Gb_x8_3200.ini --cpu-type stream -n 100000 

// Running trace cpu with a trace file and a config file
./build/dramsim3main -c configs/DDR4_8Gb_x8_3200.ini -n 100000 --cpu-type trace --trace-file sample_trace.txt


defaults:
--cpu-type = random (options - random, trace, stream)
-n = 100000

```

## Using Visual Studio Code
Just the usual way of working with a Makefile based project. Check the .vscode directory for further help.
