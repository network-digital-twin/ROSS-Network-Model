import sys
import pandas as pd
import numpy as np

def read_traces_file(filename):
    """
    Reads traces from a file and returns a pandas DataFrame.
    Assumes the format: "pid, mid, src, dest, size, time, priority"
    """
    columns = ["pid", "mid", "src", "dest", "size", "time", "priority"]
    df = pd.read_csv(filename, names=columns, delimiter=" ")
    return df

def read_stats_file(filename):
    """
    Reads statistics data from a file and returns a pandas DataFrame.
    Assumes the format: "pid, delay, drop"
    """
    columns = ["pid", "delay", "drop"]
    df = pd.read_csv(filename, names=columns, delimiter=",")
    return df

def calculate_overall_average_delay_and_jitter(traces_file, stats_file):
    traces_df = read_traces_file(traces_file)
    stats_df = read_stats_file(stats_file)
    
    # un-dropped packets
    mean_delay = stats_df[stats_df['drop']==0]['delay'].mean()
    std_delay = stats_df[stats_df['drop']==0]['delay'].std(ddof=0) # set divisor to be N, not N-1
    return mean_delay, std_delay
    
    
def calculate_detailed_average_delay_and_jitter(traces_file, stats_file):
    traces_df = read_traces_file(traces_file)
    stats_df = read_stats_file(stats_file)

    # Merge data frames on 'pid'
    merged_df = pd.merge(traces_df, stats_df, how="inner", on="pid")

    # Calculate avg delay and jitter
    undropped_merged_df = merged_df[merged_df['drop']==0]
    undropped_grouped_df = undropped_merged_df.groupby(["src", "dest"]).agg(
        avg_delay=("delay", "mean"),
        std_delay=("delay", lambda x: np.std(x, ddof=0)),  # Calculate std
        received=("delay", "size")
    )

    # Calculate drop rate
    grouped_df = merged_df.groupby(["src", "dest"]).agg(
        drop=("drop", "sum"),
        count=("drop", "size"),
    )
    grouped_df["drop_rate"] = grouped_df["drop"]/grouped_df["count"]

    # Prepare the output data, and set all NaN to 0
    merged_grouped_df = pd.merge(grouped_df, undropped_grouped_df, how="outer", on=["src", "dest"])
    merged_grouped_df[['received', 'avg_delay', 'std_delay']] = merged_grouped_df[['received', 'avg_delay', 'std_delay']].fillna(0)
    merged_grouped_df = merged_grouped_df.sort_values(by=['src', 'dest'])
    merged_grouped_df
    
    # reorder columns to be: src,dst,num_packet,avg_delay_in_ns,jitter,drop_rate
    return merged_grouped_df[['count', 'avg_delay', 'std_delay', 'drop_rate']]


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python my_script.py traces_file stats_file")
        sys.exit(1)

    traces_file = sys.argv[1]
    stats_file = sys.argv[2]

    detailed_df = calculate_detailed_average_delay_and_jitter(traces_file, stats_file)
    overall_df = calculate_overall_average_delay_and_jitter(traces_file, stats_file)

    print("Detalied average delay and jitter for each src-dest pair:")
    print(detailed_df)
    detailed_df.to_csv("detailed_stats.csv", header=None)

    print("Overall delay and jitter:")
    print(overall_df)
    pd.DataFrame([overall_df]).to_csv("overall_stats.csv", header=None, index=None)
