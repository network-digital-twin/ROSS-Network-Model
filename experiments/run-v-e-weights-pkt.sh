#!/bin/bash

BASE_PATH=$(dirname $(pwd))

OUT_PATH="${BASE_PATH}/experiments/v-e-weights-pkt"
PARTITION_PATH="${BASE_PATH}/partition/graph-for-metis-w-v-e-weight-pkt.txt"
TRACE_PATH="${BASE_PATH}/WL_generation/traces/lightweight/1000ms/trace_0_FLOW_THROUGHPUT-1250000__SIMULATION_TIME-1000000000__PAIRS_PER_SRC-1-0__MSG_SIZE-10000__PACKET_SIZE-1400__BANDWIDTH-1250000__PRIO_LEVELS-3"
ROUTE_PATH="${BASE_PATH}/WL_generation/topologies/final_topology_0"

HOST_FILE="${BASE_PATH}/experiments/hostfile-v-e-weights-pkt"

BUILD_PATH="${BASE_PATH}/build"
EXE_PATH="${BASE_PATH}/build/model/network"

mkdir ${OUT_PATH}
mkdir ${BUILD_PATH}
cd ${BUILD_PATH} && cmake .. && make

cd ${BUILD_PATH}/model


for np in 2 4 8 16 29 57
do
    for i in 0 1 2 3 4
    do
        mpirun --prefix /opt/openmpi-4.1.6 --mca btl ^openib --hostfile ${HOST_FILE} -np $np ./network \
                --partition-path=${PARTITION_PATH}.part.${np} \
                --model-home=${BASE_PATH} \
                --trace-path=${TRACE_PATH} \
                --route-path=${ROUTE_PATH} \
                --end=1000000000 --sync=3 --avl-size=22 | tee 2>&1 ${OUT_PATH}/log-$np-$i.txt
        sleep 2
    done
done