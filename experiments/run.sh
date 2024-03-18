#!/bin/bash

BASE_PATH="/home/nan42/codes-dev/ROSS-Network-Model"

BUILD_PATH=${BASE_PATH}/build
EXE_PATH=${BASE_PATH}/build/model/network

mkdir ${BUILD_PATH}
cd ${BUILD_PATH}; cmake ..; make

cd ${BUILD_PATH}/model

./network --end=1000000000
#mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib --hostfile $BASE_PATH/experiments/hostfile -np 80 ./network --end=1000000000 --sync=3 #--avl-size=20
