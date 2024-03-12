import pandas as pd

file_name = "trace_test.txt"
output_filename = "sorted_" + file_name

# Read the space-separated file
columns = ['packet_id', 'message_id', 'src', 'dest', 'size', 'time', 'priority']
df = pd.read_csv(file_name, sep=" ", header=None, names=columns)

# Now you can work with the DataFrame 'df'
print(df[:20])

# User input values
input_class = 'time'  # The 6th column name

# Sort the DataFrame based on the difference (ascending order)
df_sorted = df.sort_values(by=input_class)

# Reset the index to reorder it
df_sorted.reset_index(drop=True, inplace=True)

print(df_sorted[:20])

df_sorted.to_csv(output_filename, sep=' ', index=False, header=False)