import subprocess
import os
import pandas as pd

# Configurations
BIN_DIR = "../bin"  # Directory where benchmark outputs are stored
PROGRAM_NAMES = ["ar", "bc", "blowfish", "cem", "cuckoo", "rsa"]
VERSIONS = ["alpaca", "no_alpaca", "privatize_all", "war_only"]

# Output table initialization
columns = ["Program", "Version", "Loads", "Stores", "Bytes Loaded", "Bytes Stored", "Bytes Used"]
results = []

# Helper function to parse benchmark output
def parse_output(output):
    parsed_data = {"Loads": None, "Stores": None, "Bytes Loaded": None, "Bytes Stored": None, "Bytes Used": None}
    for line in output.splitlines():
        line = line.strip()
        if "loads" in line and "stores" in line:
            parts = line.split(",")
            parsed_data["Loads"] = int(parts[0].split()[0])
            parsed_data["Stores"] = int(parts[1].split()[0])
        elif "bytes loaded" in line and "bytes stored" in line:
            parts = line.split(",")
            parsed_data["Bytes Loaded"] = int(parts[0].split()[0])
            parsed_data["Bytes Stored"] = int(parts[1].split()[0])
        elif "bytes used" in line:
            parsed_data["Bytes Used"] = int(line.split()[0])
    return parsed_data

# Run benchmarks and collect results
for program in PROGRAM_NAMES:
    for version in VERSIONS:
        executable = os.path.join(BIN_DIR, f"benchmarks_{program}_{version}.out")
        if not os.path.isfile(executable):
            print(f"Skipping missing file: {executable}")
            continue

        try:
            print(f"Running: {executable}")
            result = subprocess.run([executable], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=60)
            if result.returncode != 0:
                print(f"Error running {executable}: {result.stderr}")
                continue
            
            parsed_data = parse_output(result.stdout)
            results.append({"Program": program, "Version": version, **parsed_data})

        except subprocess.TimeoutExpired:
            print(f"Timeout running {executable}. Skipping.")
        except Exception as e:
            print(f"Unexpected error running {executable}: {e}")

# Create a table and save as CSV
df = pd.DataFrame(results, columns=columns)
df.to_csv("benchmark_results.csv", index=False)
print("Results saved to benchmark_results.csv")
