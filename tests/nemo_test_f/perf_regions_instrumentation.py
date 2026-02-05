#!/usr/bin/env python3


import sys
import os

sys.path.append("../../scripts")
import perf_regions

pr: perf_regions.PerfRegions = perf_regions.PerfRegions(["./src/*"], output_directory="./build")
pr.fortran_region_pattern_match = {
    "include": r"^(.*)use\s+timing\s*$",  # initialization of timing
    "init": r"^(.*)call\s+timing_init\(.*\).*$",  # initialization of timing
    "finalize": r"^(.*)call\s+timing_finalize\(\s*\).*$",  # shutdown of timing
    "start": r"^(.*)call\s+timing_start\((.*)\).*$",  # start of timing
    "stop": r"^(.*)call\s+timing_stop\((.*)\).*$",  # stop of timing
}

option = "preprocess"
if len(sys.argv) > 1:
    option = sys.argv[1]


if option == "preprocess":
    print("PREPROCESS")
    pr.run_preprocessor()

elif option == "cleanup":
    print("CLEANUP: No-op for separate build dir")

else:
    print("Unsupported argument " + sys.argv[1])
