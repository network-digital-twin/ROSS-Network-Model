#!/bin/bash

BASE_PATH="/home/nan42/codes-dev/ROSS-Network-Model"

BUILD_PATH=${BASE_PATH}/build
EXE_PATH=${BASE_PATH}/build/model/network

mkdir ${BUILD_PATH}
cd ${BUILD_PATH} && cmake .. && make

cd ${BUILD_PATH}/model

./network --shaper-interval=100000

