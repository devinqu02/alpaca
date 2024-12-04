import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Load results from the CSV file
data_file = "benchmark_results.csv"
df = pd.read_csv(data_file)

# Filter for relevant versions
filtered_versions = [ "no_alpaca", "alpaca"]
colors = {
    "no_alpaca": ("#32CD32", "#228B22"),
    "alpaca": ("#87CEEB", "#1E3A8A"),
}
df = df[df["Version"].isin(filtered_versions)]

# Normalize data such that "privatize_all" for each program is 1
normalized_df = df.copy()

for program in df["Program"].unique():
    # Normalize Loads and Stores by their sum
    loads_value = df[(df["Program"] == program) & (df["Version"] == "no_alpaca")]["Loads"].values
    stores_value = df[(df["Program"] == program) & (df["Version"] == "no_alpaca")]["Stores"].values
    if len(loads_value) > 0 and loads_value[0] != 0 and len(stores_value) > 0 and stores_value[0] != 0:
        loads_stores_sum = loads_value[0] + stores_value[0]
        if loads_stores_sum != 0:
            normalized_df.loc[normalized_df["Program"] == program, "Loads"] /= loads_stores_sum
            normalized_df.loc[normalized_df["Program"] == program, "Stores"] /= loads_stores_sum

    # Normalize Bytes Loaded and Bytes Stored by their sum
    bytes_loaded_value = df[(df["Program"] == program) & (df["Version"] == "no_alpaca")]["Bytes Loaded"].values
    bytes_stored_value = df[(df["Program"] == program) & (df["Version"] == "no_alpaca")]["Bytes Stored"].values
    if len(bytes_loaded_value) > 0 and bytes_loaded_value[0] != 0 and len(bytes_stored_value) > 0 and bytes_stored_value[0] != 0:
        bytes_sum = bytes_loaded_value[0] + bytes_stored_value[0]
        if bytes_sum != 0:
            normalized_df.loc[normalized_df["Program"] == program, "Bytes Loaded"] /= bytes_sum
            normalized_df.loc[normalized_df["Program"] == program, "Bytes Stored"] /= bytes_sum

    # Normalize Bytes Used (same as before)
    bytes_used_value = df[(df["Program"] == program) & (df["Version"] == "no_alpaca")]["Bytes Used"].values
    if len(bytes_used_value) > 0 and bytes_used_value[0] != 0:
        normalized_df.loc[normalized_df["Program"] == program, "Bytes Used"] /= bytes_used_value[0]

# Plot 1: Stacked bars for Bytes Loaded + Bytes Stored
fig, ax = plt.subplots(figsize=(12, 8))
programs = normalized_df["Program"].unique()
x = np.arange(len(programs))
bar_width = 0.25

for i, version in enumerate(filtered_versions):
    bytes_loaded = [
        normalized_df[(normalized_df["Program"] == program) & (normalized_df["Version"] == version)]["Bytes Loaded"].mean()
        for program in programs
    ]
    bytes_stored = [
        normalized_df[(normalized_df["Program"] == program) & (normalized_df["Version"] == version)]["Bytes Stored"].mean()
        for program in programs
    ]
    ax.bar(
        x + i * bar_width, bytes_loaded, width=bar_width, label=f"{version} - Bytes Loaded", color = colors[version][1]
    )
    ax.bar(
        x + i * bar_width, bytes_stored, width=bar_width, bottom=bytes_loaded, label=f"{version} - Bytes Stored", color = colors[version][0]
    )

ax.set_xlabel("Benchmark Name")
ax.set_ylabel("Memory Accessed (Normalized)")
ax.set_title("Memory Accessed by Program and Version (Overhead)")
ax.set_xticks(x + bar_width)
ax.set_xticklabels(programs, rotation=45)
ax.legend(title="Version")
plt.tight_layout()
plt.savefig("graphs/bytes_loads_stores_stacked_overhead.png")
plt.show()

# Plot 2: Stacked bars for Load Counts + Store Counts
fig, ax = plt.subplots(figsize=(12, 8))
for i, version in enumerate(filtered_versions):
    loads = [
        normalized_df[(normalized_df["Program"] == program) & (normalized_df["Version"] == version)]["Loads"].mean()
        for program in programs
    ]
    stores = [
        normalized_df[(normalized_df["Program"] == program) & (normalized_df["Version"] == version)]["Stores"].mean()
        for program in programs
    ]
    ax.bar(
        x + i * bar_width, loads, width=bar_width, label=f"{version} - Loads", color = colors[version][1]
    )
    ax.bar(
        x + i * bar_width, stores, width=bar_width, bottom=loads, label=f"{version} - Stores", color = colors[version][0]
    )

ax.set_xlabel("Benchmark Name")
ax.set_ylabel("# of Memory Accesses (Normalized)")
ax.set_title("# of Memory Accesses by Program and Version (Overhead)")
ax.set_xticks(x + bar_width)
ax.set_xticklabels(programs, rotation=45)
ax.legend(title="Version")
plt.tight_layout()
plt.savefig("graphs/counts_loads_stores_stacked_overhead.png")
plt.show()

# Plot 3: Bars for Memory Used
fig, ax = plt.subplots(figsize=(12, 8))
for i, version in enumerate(filtered_versions):
    memory_used = [
        normalized_df[(normalized_df["Program"] == program) & (normalized_df["Version"] == version)]["Bytes Used"].mean()
        for program in programs
    ]
    ax.bar(
        x + i * bar_width, memory_used, width=bar_width, label=version, color = colors[version][1]
    )

ax.set_xlabel("Benchmark Name")
ax.set_ylabel("Memory Used (Normalized)")
ax.set_title("Memory Used by Program and Version (Overhead)")
ax.set_xticks(x + bar_width)
ax.set_xticklabels(programs, rotation=45)
ax.legend(title="Version")
plt.tight_layout()
plt.savefig("graphs/memory_used_overhead.png")
plt.show()

print("Graphs generated and saved as PNG files.")
