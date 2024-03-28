#!/bin/bash

BASE_PATH="/home/nan42/codes-dev/ROSS-Network-Model"

BUILD_PATH=${BASE_PATH}/build
EXE_PATH=${BASE_PATH}/build/model/network

mkdir ${BUILD_PATH}
cd ${BUILD_PATH} && cmake .. && make

cd ${BUILD_PATH}/model

#./network --end=1000000000
#mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib --hostfile $BASE_PATH/experiments/hostfile -np 32 ./network --avl-size=20 --end=1000000000 --sync=3
#exit

pe="${1:-8}"
avl="${2:-22}"
#echo "value of pe=$pe"
echo "value of avl-size=$avl"

#mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np $pe ./network --end=1000000000 --sync=3 --avl-size=$avl | tee 2>&1 log-$pe-$i.txt
#exit


for np in 2 4 8 16
do
    for i in 0 1 2 3 4
    do
        mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np $np ./network --end=1000000000 --sync=3 --avl-size=22 | tee 2>&1 log-$np-$i.txt
        sleep 2
    done
done	


np=29
for i in 0 1 2 3 4
do
        mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib --hostfile $BASE_PATH/experiments/hostfile -np $np ./network --end=1000000000 --sync=3 --avl-size=20 | tee 2>&1 log-$np-$i.txt
        sleep 2
done
#mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np 4 ./network --end=1000000000 --sync=3 --avl-size=22
#mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib --hostfile $BASE_PATH/experiments/hostfile -np 80 ./network --end=1000000000 --sync=3 #--avl-size=20
