import json

LP_switch_mapping_file = 'lp_to_switch_mapping.txt'
graph_edge_file = 'edges.json'
weights_file = None
output_file = 'metis/graph_metis.txt'

def get_lp_of_switch(switch_id, switch_to_lp_map):
    if is_switch_in_map(switch_id, switch_to_lp_map) == False:
        raise Exception("Invalid switch id! Not found in switch-lp map", switch_id)
    return switch_to_lp_map[switch_id]


def is_switch_in_map(switch_id, switch_to_lp_map):
    return switch_id in switch_to_lp_map


def load_switch_to_lp_map(mapping_file):
    with open(mapping_file, 'r') as f:
        switch_to_lp_map = {}
        lp_to_switch_map = {}
        lines = f.readlines()
        # Load the mapping file
        for lpid in range(len(lines)):
            switch_id = int(lines[lpid])
            # print(switch_id)
            switch_to_lp_map[switch_id] = lpid
            lp_to_switch_map[lpid] = switch_id
        return switch_to_lp_map, lp_to_switch_map


def load_edges(file):
    with open(file, 'r') as f:
        edges = json.load(f)
        return edges
        

def process_edges(all_edges, switch_to_lp_map):
    subnet_edges_metis = {}
    
    # prepare edges for metis
    for edge in all_edges:
        # Filter out the edges that are not in the switch_to_lp_map
        if(is_switch_in_map(edge['srcNode'], switch_to_lp_map) == False):
            continue
            
        src_switch = edge['srcNode']
        dst_switch = edge['destNode']
        try:
            src_lp = get_lp_of_switch(src_switch, switch_to_lp_map)
            dst_lp = get_lp_of_switch(dst_switch, switch_to_lp_map)
        except Exception as e:
            print("----------- [Ignored] Exception: ", e)
            continue
        
        if(src_lp not in subnet_edges_metis):
            subnet_edges_metis[src_lp] = {"switch_id": src_switch, "connected_lp_id": [], "connected_switch_id": []}
        if (dst_lp not in subnet_edges_metis[src_lp]["connected_lp_id"]): # Avoid adding duplicate edges
            subnet_edges_metis[src_lp]["connected_lp_id"].append(dst_lp)
            subnet_edges_metis[src_lp]["connected_switch_id"].append(dst_switch)
        else:
            print(f"Duplicate edge found, ignoring: {src_lp}->{dst_lp} (switch id {src_switch} -> {dst_switch})")

    # check the validity of the data
    for lp in subnet_edges_metis:
        for connected_lp, connected_switch in zip(subnet_edges_metis[lp]["connected_lp_id"], subnet_edges_metis[lp]["connected_switch_id"]):
            if lp not in subnet_edges_metis[connected_lp]["connected_lp_id"]:
                switch = subnet_edges_metis[lp]['switch_id']
                print(f"Error: lpid {lp}->{connected_lp} (switch id {switch} -> {connected_switch}) is found, but {connected_lp}->{lp} (switch id {connected_switch} -> {switch}) is not found in  subnet_edges_metis")
                print(f"Adding {connected_lp}->{lp} (switch id {connected_switch} -> {switch}) to subnet_edges_metis")
                subnet_edges_metis[connected_lp]["connected_lp_id"].append(lp)
    print("----------- Edges processed successfully -----------")
    return subnet_edges_metis

"""
process_edges is a dict, the value of each key should look like this: {'switch_id': 1, 'connected_lp_id': [4750, 3004], 'connected_switch_id': [8621, 5455]}
"""
def write_to_metis_file(processed_edges: dict, output_file):
    with open(output_file, 'w') as f:
        num_edges = 0
        num_vertices = len(processed_edges)
        lines = []
        for i in range(len(processed_edges)):
            # according to METIS, the indices should start from 1, not 0. So we need to add 1 to all the index
            line = ' '.join(str(lpid + 1) for lpid in processed_edges[i]['connected_lp_id']) 
            num_edges += len(processed_edges[i]['connected_lp_id'])
            lines.append(line)
        
        f.write(f"{num_vertices} {num_edges//2}\n")
        for line in lines:
            f.write(line + "\n")
        print("Number of edges:", num_edges//2)
        print("Number of vertices:", num_vertices)
        print("Done writing to file:", output_file)


if __name__ == "__main__":
    switch_to_lp_map, lp_to_switch_map = load_switch_to_lp_map(LP_switch_mapping_file)
    print("switch_to_lp_map length:", len(switch_to_lp_map))
    print("lp_to_switch_map length:", len(lp_to_switch_map))
    edges = load_edges(graph_edge_file)

    metis = process_edges(edges, switch_to_lp_map)
    print("----------- number of nodes for metis:", len(metis))

    write_to_metis_file(metis, output_file)

    


