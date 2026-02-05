#!/usr/bin/env python3


import sys
import os

sys.path.append("../../scripts")
import perf_regions

pr: perf_regions.PerfRegions = perf_regions.PerfRegions(["./src/*"], output_directory="./build")

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
