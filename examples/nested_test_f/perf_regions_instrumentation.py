#!/usr/bin/env python3


import sys
sys.path.append('../../scripts')
import perf_regions


pr: perf_regions.PerfRegions = perf_regions.PerfRegions("./*", output_directory="./build_perf_regions")
# pr: perf_regions.PerfRegions = perf_regions.PerfRegions("./*")


option = "preprocess"
if len(sys.argv) > 1:
    option = sys.argv[1]


if option == 'preprocess':
    print("PREPROCESS")
    pr.run_preprocessor()

elif option == 'cleanup':
    print("CLEANUP")
    pr.remove_perf_regions_annotations()

else:
    print("Unsupported argument "+sys.argv[1])

