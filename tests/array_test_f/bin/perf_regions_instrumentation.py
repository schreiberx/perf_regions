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

if option == "preprocess":
    print("PREPROCESS")
    pr.run_preprocessor()

elif option == "cleanup":
    print("CLEANUP")
    pr.remove_perf_regions_annotations()

else:
    print("Unsupported argument " + sys.argv[1])
