#!/bin/bash

BASE_PATH="/home/nan42/codes-dev/ROSS-Network-Model"

BUILD_PATH=${BASE_PATH}/build
EXE_PATH=${BASE_PATH}/build/model/network

mkdir ${BUILD_PATH}
cd ${BUILD_PATH} && cmake .. && make

cd ${BUILD_PATH}/model

#./network --end=1000000000
for i in 0 1 2 3 4
do
        mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np 2 ./network --end=1000000000 --sync=3 --avl-size=22 | tee 2>&1 log-2-$i.txt
        sleep 2
done

for i in 0 1 2 3 4
do
        mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np 4 ./network --end=1000000000 --sync=3 --avl-size=22 | tee 2>&1 log-4-$i.txt
        sleep 2
done

for i in 0 1 2 3 4
do 
	mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np 8 ./network --end=1000000000 --sync=3 --avl-size=22 | tee 2>&1 log-8-$i.txt
	sleep 2
done
for i in 0 1 2 3 4
do
        mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np 16 ./network --end=1000000000 --sync=3 --avl-size=20 | tee 2>&1 log-16-$i.txt
        sleep 2
done
#mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib -np 4 ./network --end=1000000000 --sync=3 --avl-size=22
#mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib --hostfile $BASE_PATH/experiments/hostfile -np 80 ./network --end=1000000000 --sync=3 #--avl-size=20
