# Introduction

This is an example model for use with [ROSS](http://github.com/carothersc/ROSS), the parallel discrete event simulation system at Rensselaer Polytechnic Institute.

This model simulates a simple postal network of terminals and switches. Terminals send packets which are delivered to specific assigned switches. The switches process packets, determining if each message that they receive can be delivered to a local terminal or if it must be routed to another switch.

This example model shows simple usage of scheduling new events, altering and reading message states, mapping of multiple LPs in a PDES system, and other intricacies of ROSS model development.


# Installation

This model requires the installation of [ROSS](http://github.com/carothersc/ROSS): 
```bash
cd ~/directory-of-builds/
git clone https://github.com/ROSS-org/ROSS.git
mkdir build-ross
cd build-ross

cmake -DCMAKE_INSTALL_PREFIX:path=`pwd` -DCMAKE_C_COMPILER=$(which mpicc) -DCMAKE_CXX_COMPILER=$(which mpicxx) ../ROSS
make install
```

Then modify cmake/FindROSS.cmake to point to the ROSS build directory. 
Modify the path of `HINTS` in the following lines to point to the build-ross directory.
```cmake
FIND_PATH(WITH_ROSS_PREFIX
    NAMES include/ross.h
		HINTS "../build-ross"
)
```

Then build the model:
```bash
cd ~/directory-of-builds/
mkdir build
cd build
cmake ../
make
```

The executable will be placed in the `build/model` directory.

To run the simulation:
```bash
cd build/model
mpirun -n 2 ./network --sync=3
```