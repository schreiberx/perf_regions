#! /usr/bin/env python3

import os
import subprocess
import sys
import pandas as pd

os.environ["LD_LIBRARY_PATH"] = f"../../build:{os.environ.get('LD_LIBRARY_PATH', '')}"

num_threads = 18
# num_threads = 24
os.environ["OMP_NUM_THREADS"] = f"{num_threads}"
os.environ["OMP_PROC_BIND"] = "close"
os.environ["OMP_DISPLAY_ENV"] = "verbose"
os.environ["OMP_DISPLAY_AFFINITY"] = "true"

places = ",".join(["{"+str(_)+"}" for _ in range(num_threads)])
os.environ["OMP_PLACES"] = places


print("OMP_NUM_THREADS=", os.environ["OMP_NUM_THREADS"])
print("OMP_PROC_BIND=", os.environ["OMP_PROC_BIND"])
print("OMP_DISPLAY_ENV=", os.environ["OMP_DISPLAY_ENV"])
print("OMP_DISPLAY_AFFINITY=", os.environ["OMP_DISPLAY_AFFINITY"])
print("OMP_PLACES=", os.environ["OMP_PLACES"])

#
# See also
# https://www.intel.com/content/www/us/en/docs/vtune-profiler/cookbook/2024-1/analyze-uncore-perfmon-events-for-intel-ddio.html
#

#
# We need to determine the number of Integrated Memory Controllers (IMCs) on the system
#

#
# Prerequisites:
#
# sudo-g5k bash -c 'echo "-1" > /proc/sys/kernel/perf_event_paranoid'
#

#
# `hwloc-info` / `likwid-topology`
#
# - 2 NUMA nodes
#

#
# `hwloc-ls --of txt` / `likwid-topology`
#
# - P0-P23 on first NUMA node
# - P24-P47 on second NUMA node
#

#
# Now we have to figure out how many memory controllers (IMCs) are present per NUMA node
#
# - https://github.com/intel/pcm
#
# First, enable msr:
#
# $ sudo-g5k modprobe msr
# $ ./pcm-core 5 1 -c 2>&1 | grep memory
#
# => 4 memory controllers per socket
#

if 1:
    # Setup up events on cacheblock granularity
    events_cacheblocks = []

    #
    # First 6 IMCs are for first NUMA node.
    # Second 6 IMCs are for the second NUMA node.
    #
    # IMC 0 handles channels 0 and 1.
    # IMC 1 handles channels 2 and 3.
    # IMC 3 handles channels 4 and 5.
    # IMC 4 handles channels 6 and 7.
    #

    icms_ = range(6)
    #icms_ = [0, 1, 3, 4]

    for imc in icms_:
        cpu = 0
        events_cacheblocks += [f"skx_unc_imc{imc}::UNC_M_CAS_COUNT:ALL:cpu={cpu}"]

    if 0:
        for imc in icms_:
            cpu = 24
            events_cacheblocks += [f"skx_unc_imc{6+imc}::UNC_M_CAS_COUNT:ALL:cpu={cpu}"]

else:
    events_cacheblocks = ["perf::LLC-LOAD-MISSES"]

print("")
for e in events_cacheblocks:
    print(f"Using event: {e}")
print("")

cacheblock_size = 64

perf_regions_counters = ",".join(events_cacheblocks) + ",WALLCLOCKTIME"
os.environ["PERF_REGIONS_COUNTERS"] = perf_regions_counters
print(f"PERF_REGIONS_COUNTERS={perf_regions_counters}")


print("")
print("*****************************************************")
print("$ make stream_c_perfregions")

result = subprocess.run(["make", "stream_c_perfregions"], capture_output=True, text=True)
if result.returncode != 0:
    print(result.stdout)
    print(result.stderr, file=sys.stderr)
    sys.exit(1)


print("")
print("*****************************************************")
print("./stream_c_perfregions")

try:
    output = subprocess.check_output(["./stream_c_perfregions"], stderr=subprocess.STDOUT, text=True)
except subprocess.CalledProcessError as e:
    output = e.output + "\nERROR"

if "ERROR" in output:
    print(output)
    print("Error: Detected ERROR in output when running './stream_c_perfregions'", file=sys.stderr)
    sys.exit(1)

print("")
print("*****************************************************")
print("PERF REGION OUTPUT:")

print(output)

def cleanup_events(cols):
    cols = [_.replace("::UNC_M_CAS_COUNT:ALL", "")for _ in cols]
    cols = [_.replace("icx_unc_", "")for _ in cols]

    return cols


pr_tag = "[MULE] perf_regions: "
start_data = False
data_table = []
for line in output.splitlines():
    if not line.startswith(pr_tag):
        continue

    cols = line[len(pr_tag) :].split()
    if cols[0] == "Section":
        cols = cleanup_events(cols)
        data_table.append(cols)
        start_data = True
        continue

    if not start_data:
        continue

    cols[1:] = [float(_) for _ in cols[1:]]
    data_table.append(cols)

events_cacheblocks = cleanup_events(events_cacheblocks)
# Convert data_table to a pandas DataFrame and print it
df = pd.DataFrame(data_table[1:], columns=data_table[0])

print("")
print("Pandas DataFrame of perf region output:")
print(df)

papi_cacheblocks_sum = values = df[events_cacheblocks[0]].astype(float).tolist()
for event in events_cacheblocks[1:]:
    values = df[event].tolist()
    for i, value in enumerate(values):
        papi_cacheblocks_sum[i] += values[i]

counter_values = df["COUNTER"].astype(float).tolist()
wallclocktime_values = df["WALLCLOCKTIME"].astype(float).tolist()
print(wallclocktime_values)

bw = []
for i, accesses in enumerate(papi_cacheblocks_sum):
    wallclocktime = wallclocktime_values[i]

    value = accesses * cacheblock_size / wallclocktime * 1e-9 # in GB/s
    # value /= counter_values[i]
    bw.append(value)

df["BANDWIDTH"] = bw

print("")
print("DataFrame:")

out = df.set_index("Section").T
for col in out.columns:
    out[col] = out[col].apply(lambda x: "{:.4e}".format(x))

print(out.to_string())

if 1:
    df_indexed = df.set_index("Section")
    bw_list = df_indexed.loc[["copy", "scale", "add", "triad"], "BANDWIDTH"].tolist()

    bw_avg = sum(bw_list)/len(bw_list)
    bw_total = df_indexed.loc["total", "BANDWIDTH"]


    print("")
    print(f"Total bandwidth: {bw_total} GB/s")
    print(f"Avg bandwidth: {bw_avg} GB/s")
    print("")

