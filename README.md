# Introduction

This repo contains the source code of **Quaint** and the data used in the paper. 

**Code and data for the experiments conducted in the paper are stored in different branches**:
- For the analysis of accuracy and efficiency compared to INET, see the branch [`experiments-INET`](https://github.com/network-digital-twin/ROSS-Network-Model/tree/experiments-INET)
- For the scalability characterisation of **Quaint**, see the branch [`experiments-metis`](https://github.com/network-digital-twin/ROSS-Network-Model/tree/experiments-metis)

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
