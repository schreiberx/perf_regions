#! /usr/bin/env python3

import os
import subprocess
import sys
import pandas as pd


os.environ["LD_LIBRARY_PATH"] = f"../../build:{os.environ.get('LD_LIBRARY_PATH', '')}"
os.environ["PERF_REGIONS_COUNTERS"] = "PAPI_L3_TCM,PAPI_L2_TCM,WALLCLOCKTIME"

def run_make():
	result = subprocess.run(["make", "stream_c_perfregions"], capture_output=True, text=True)
	if result.returncode != 0:
		print(result.stdout)
		print(result.stderr, file=sys.stderr)
		sys.exit(1)

def run_stream():
	try:
		output = subprocess.check_output(["./stream_c_perfregions"], stderr=subprocess.STDOUT, text=True)
	except subprocess.CalledProcessError as e:
		output = e.output + "\nERROR"
	return output

run_make()
OUTPUT = run_stream()

if "ERROR" in OUTPUT:
	print(OUTPUT)
	print("Error: Detected ERROR in output when running './stream_c_perfregions'", file=sys.stderr)
	sys.exit(1)

print(OUTPUT)
print()
print("*****************************************************")
print("* PERF REGION OUTPUT *")
print("*****************************************************")

pr_tag = "[MULE] perf_regions: "
start_data = False
data_table = []
print("test\tMISS\tTIME\tBW")
for line in OUTPUT.splitlines():
	if not line.startswith(pr_tag):
		continue

	cols = line[len(pr_tag):].split()
	if cols[0] == "Section":
		data_table.append(cols)
		start_data = True
		continue

	if not start_data:
		continue

	data_table.append(cols)


# Convert data_table to a pandas DataFrame and print it
df = pd.DataFrame(data_table[1:], columns=data_table[0])
print("="*80)
print("| WARNING: TCM counters may not be accurate on some systems. |")
print("| WARNING: In addition, also victimed cache lines need to be taken into account (not yet done). |")
print("="*80)
print("\nPandas DataFrame of perf region output:")
print(df)


papi_l3_tcm_values = df["PAPI_L3_TCM"].astype(float).tolist()
counter_values = df["COUNTER"].astype(float).tolist()
wallclocktime_values = df["WALLCLOCKTIME"].astype(float).tolist()

cacheblock_size = 64

bw = []
for i in range(len(papi_l3_tcm_values)):
	misses = papi_l3_tcm_values[i]
	wallclocktime = wallclocktime_values[i]

	# We do not divide by the counter since it's taken into account in the misses and the wallclock time.
	# counter = counter_values[i]
	
	value = misses*cacheblock_size / (wallclocktime*1024.0*1024.0)
	bw.append(value)

df["BANDWIDTH"] = bw

print("\nDataFrame:")
print(df)
