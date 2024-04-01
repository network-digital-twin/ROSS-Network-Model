# Introduction

This branch contains the source code and data for the comparison with INET. 


# Installation

This model requires the installation of [ROSS](http://github.com/carothersc/ROSS): 
```bash
cd <YOUR-WORKING-DIRECTORY>
git clone https://github.com/ROSS-org/ROSS.git
mkdir build-ross
cd build-ross

cmake -DCMAKE_INSTALL_PREFIX:path=`pwd` -DCMAKE_C_COMPILER=$(which mpicc) -DCMAKE_CXX_COMPILER=$(which mpicxx) ../ROSS
make install
```

Then clone this project:
```bash
cd .. # should be in <YOUR-WORKING-DIRECTORY>
git clone git@github.com:network-digital-twin/ROSS-Network-Model.git
```

Now run `ls`, your current working directory should contains the following three sub-folders: 
```
ROSS  ROSS-Network-Model  build-ross
```

Then checkout to the `experiments-INET` branch:
```bash
cd ROSS-Network-Model
git checkout experiments-INET
```

(Optional) If you place the ROSS build folder (`build-ross`) to a diffrent path from the above instructions, then you need to modify `cmake/FindROSS.cmake` as follows to point to the ROSS build directory. **Otherwise, skip this step.**

Modify the path of `HINTS` in the following lines to point to the build-ross directory. 
```cmake
FIND_PATH(WITH_ROSS_PREFIX 
        NAMES include/ross.h
        HINTS "../build-ross"
)
```


Then build the model:
```bash
# now should be in ROSS-Network-Model
mkdir build
cd build
cmake ../
make
```
The executable will be placed in the `build/model` directory.

## Run the simulation

This branch is intended to evaluate the accuracy and efficiency of **Quaint** compared to INET

First unzip the topology data:
```bash
cd ../WL_generation/topologies/
unzip final_topology_0.zip
```


Use `run.sh`  to run the experiments:
First change the `--shaper-interval` in `run.sh` as needed. We’ve tested 100000, 500000, 1000000. 
To run the experiment:
```bash
cd ../../experiments/
```
```bash
for i in 0 1 2 3 4; do ./run.sh | tee 2>&1 log-$i.txt; sleep 2; done
```
All results were collected manually to [`experiments/results-exp1`](experiments/results-exp1)

