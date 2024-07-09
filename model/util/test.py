import json

# 假设你的JSON文件名为data.json
filename = '/home/nan42/codes-dev/ROSS-Network-Model/data/zte_parsed_data_06242024/nkddata_integer/3446.json'

# 用于存储不同nextHopId的集合
unique_next_hop_ids = set()

# 打开并读取JSON文件
with open(filename, 'r') as file:
    data = json.load(file)

    # 遍历routing列表中的每个元素
    for route in data.get('routing', []):
        # 将nextHopId添加到集合中
        unique_next_hop_ids.add(route.get('nextHopId'))

# 打印不同nextHopId的数量
print(f"The number of unique nextHopIds is: {len(unique_next_hop_ids)}")
print(unique_next_hop_ids)
