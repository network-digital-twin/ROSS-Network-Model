import json
import os
import math
import random
import numpy as np

subgraph = 0    # the subgraph for witch the trace will generate packets for

out_name = f'traces/trace_{subgraph}'                                       # name of saved trace
config_path = os.getcwd() + f'/topologies/final_topology_{subgraph}/'       # path to switch config files for topology
data_path = 'data/reindexed.json'  # path to file containing topology

FLOW_THROUGHPUT = 12_500_000           # BYTES PER SECOND
SIMULATION_TIME = 1_000_000               # Ns
PAIRS_PER_SRC = {'mu': 1, 'sigma': 0}   # NORMAL DIST
MSG_SIZE = 50_000                       # BYTES
PACKET_SIZE = 1400                      # BYTES
BANDWIDTH = 12_500_000                  # BYTES
PRIO_LEVELS = 3                         # QOS PRIORITIES

parameters = (f"FLOW_THROUGHPUT:{FLOW_THROUGHPUT}__"
          f"SIMULATION_TIME:{SIMULATION_TIME}__"
          f"PAIRS_PER_SRC:{tuple(PAIRS_PER_SRC.values())}__"
          f"MSG_SIZE:{MSG_SIZE}__"
          f"PACKET_SIZE:{PACKET_SIZE}__"
          f"BANDWIDTH:{BANDWIDTH}__"
          f"PRIO_LEVELS:{PRIO_LEVELS}")

print(parameters)

s_to_ns = lambda x : int(x * math.pow(10,9))
ns_to_s = lambda x : x * math.pow(10,-9)

configs = os.listdir(config_path)

with open(data_path,'r') as f:
    data = json.load(f)

nodes = {}
for node in data['nodeList']:
    nodes[node['id']] = node

sources = [] # access
destinations = [] # kernel or mixed 

for config in configs:
    id = int(config.split('.')[0])

    if nodes[id]['deviceLevel'] == 'Access':
        sources.append(id)
    else:
        destinations.append(id) 

print("num src:",len(sources), "|| num dests:",len(destinations))

# generate flows
flows = {}
for src in sources:
    num_pairs = int(random.normalvariate(**PAIRS_PER_SRC))
    while num_pairs <= 0:
        num_pairs = int(random.normalvariate(**PAIRS_PER_SRC))
    
    flows[src] = random.sample(sources, k=num_pairs)

def generate_messages_for_flow(duration_ns):
    total_bytes_for_duration = ns_to_s(duration_ns) * FLOW_THROUGHPUT

    sizes = []

    while sum(sizes) < total_bytes_for_duration:
        sizes.append(int(random.expovariate(1/MSG_SIZE)))
    
    times = np.linspace(0, duration_ns, len(sizes))
    times = [int(x) for x in times]

    return times, sizes

message_id = 0
messages = []
for src, pairs in flows.items():
     for dst in pairs:
        flow_messages = generate_messages_for_flow(SIMULATION_TIME)

        for size, time in zip(*flow_messages):
            tos = random.randint(0, PRIO_LEVELS-1)

            messages.append({
                'message_id': message_id,
                'src': src,
                'dst': dst,
                'size': size,
                'timestamp':time,
                'tos': tos
            })

        message_id += 1

messages = sorted(messages, key=lambda x:x['timestamp'])

unique_packet_id = 0

f =  open(out_name+'_'+parameters, 'w')

for msg in messages:
    message_id, src, dst, size, timestamp, tos = msg.values()

    num_packets = math.ceil(size / PACKET_SIZE) # ceil -> padding last packet to always be PACKET_SIZE
    
    packet_time = timestamp
    packets = []
    for _ in range(num_packets):
        packet_info = (
            f"{str(unique_packet_id)} "
            f"{str(message_id)} "
            f"{str(src)} "
            f"{str(dst)} "
            f"{str(PACKET_SIZE)} "
            f"{str(packet_time)} "
            f"{str(tos)}\n"
        )

        f.write(packet_info)

        packet_time += PACKET_SIZE / BANDWIDTH
        unique_packet_id += 1 

f.close()