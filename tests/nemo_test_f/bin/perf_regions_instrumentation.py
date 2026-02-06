#!/usr/bin/env python3


import sys
import os

sys.path.append(os.path.join(os.path.dirname(__file__), "../../../scripts"))
import perf_regions

# Process files in build/ in-place
args = sys.argv[1:]
targets = ["./build/*"]
option = "preprocess"

if len(args) > 0:
    # Check if last arg is option
    if args[-1] in ["preprocess", "cleanup"]:
        option = args.pop()  # remove it

    # If args left, they are targets
    if len(args) > 0:
        targets = args

pr: perf_regions.PerfRegions = perf_regions.PerfRegions(targets)
pr.fortran_region_pattern_match = {
    "include": r"^(.*)use\s+timing\s*$",  # initialization of timing
    "init": r"^(.*)call\s+timing_init\(.*\).*$",  # initialization of timing
    "finalize": r"^(.*)call\s+timing_finalize\(\s*\).*$",  # shutdown of timing
    "start": r"^(.*)call\s+timing_start\((.*)\).*$",  # start of timing
    "stop": r"^(.*)call\s+timing_stop\((.*)\).*$",  # stop of timing
}

if option == "preprocess":
    print("PREPROCESS")
    pr.run_preprocessor()

elif option == "cleanup":
    print("CLEANUP")
    pr.remove_perf_regions_annotations()

else:
    print("Unsupported argument " + sys.argv[1])
