# dramcore
Model the timing paramaters and memory controller behaviour for several DRAM protocols such as DDR3, DDR4, LPDDR3, LPDDR4, GDDR5, HBM.


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

# Build dramcore and dramcoretest executables
make -j4

```

The build process creates `dramcoremain` and `dramcoretest` executables in the build directory.
It also creates `libdramcore.so` shared library in the parent directory. 


### Running
This is still in development. Everything is still hardcoded. Current hacky way of running is

```
# Copy the sample trace file into the directory from which the dramcoremain executable is being run
cp ./../sample_trace.txt .

# Run the dramcore executable
./dramcoremain

# Run the dramcoretest executable (No actual tests are currently written :P)
./dramcoretest

```

## Using Clion IDE (preferred way)
Clion supports a CMake based buildsystem natively. Infact, being able to use Clion was the motivation behind moving
from a handwritten Makefile to a CMake based build system.

Import the project from Clion. This will create a `cmake-build-debug` directory. Use this as the new build
and run directory.


*Note - While CMake is a paid IDE, it provides a free student license.*


## Using Visual Studio Code
Just the usual way of working with a Makefile based project. Check the .vscode directory for further help.
