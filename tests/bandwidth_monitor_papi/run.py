#!/usr/bin/env python3
import os
import subprocess
import sys
import re

# Try to auto-detect using PAPI
detected_imcs = []

try:
    cmd = ["papi_native_avail", "-i", "UNC_M_CAS_COUNT", "--noqual"]
    output = subprocess.check_output(cmd, universal_newlines=True, stderr=subprocess.STDOUT)
    for line in output.splitlines():
        if "UNC_M_CAS_COUNT" in line:
            # Clean line and extract event
            # Format: | event_name |
            clean = line.replace("|", "").strip()
            if "::UNC_M_CAS_COUNT" in clean:
                detected_imcs.append(clean)
except Exception as e:
    raise Exception(f"PAPI detection failed: {e}")

# Default/Other architecture
event_prefix = "icx_unc_imc"
num_imc = 6
num_domains = 1
num_cpus_per_domain = 0

if len(detected_imcs) > 0:
    print(f"PAPI Analysis: Found {len(detected_imcs)} IMC events:")
    print(detected_imcs)
    num_imc = len(detected_imcs)
    # Extract prefix from first event (e.g. skx_unc_imc0 -> skx_unc_imc)
    evt = detected_imcs[0].split("::")[0]
    event_prefix = re.sub(r'\d+$', '', evt)

if 1:
    # Check for CPU model
    processor_brand = "unknown"
    try:
        with open('/proc/cpuinfo', 'r') as f:
            for line in f:
                if "model name" in line:
                    processor_brand = line.split(":")[1].strip()
                    break
    except IOError:
        pass

    if "DUMMY" in processor_brand:
        # Setup custom settings for particular CPU
        pass


# Define events for IMCs 0 to 5
imc_events = []

for d in range(num_domains):
    for i in range(num_imc):
        cpu = d * num_cpus_per_domain
        base_event = f"{event_prefix}{i}::UNC_M_CAS_COUNT"
        imc_events.append(f"{base_event}.RD:cpu={cpu}")
        imc_events.append(f"{base_event}.WR:cpu={cpu}")

for i in imc_events:
    print(f"Using event: {i}")

env_string = ",".join(imc_events)

env = os.environ.copy()
env["PERF_REGIONS_COUNTERS"] = env_string

print(f"Set PERF_REGIONS_COUNTERS={env_string}")
print("Running bandwidth_monitor_papi...")

try:
    # Run the program
    call_args = ["./bandwidth_monitor_papi", "-i", "0.3"] + sys.argv[1:]
    ret = subprocess.call(call_args, env=env)
    sys.exit(ret)
except KeyboardInterrupt:
    print("\nInterrupted by user.")
    sys.exit(0)
except Exception as e:
    print(f"Error running program: {e}")
    sys.exit(1)
