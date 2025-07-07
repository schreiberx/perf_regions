#!/usr/bin/env python3


import sys
sys.path.append('../../scripts')

import perf_regions

pf = perf_regions.perf_regions(
        ["./"],    # list with source directories
        [
            ".*#pragma perf_regions init .*",        # initialization of timing
            ".*#pragma perf_regions init_mpi (.*).*",  # initialization of timing when using mpi
            ".*#pragma perf_regions finalize.*",    # shutdown of timing

            ".*#pragma perf_regions include.*",    # include part

            ".*#pragma perf_regions start (.*).*",    # start of timing
            ".*#pragma perf_regions stop (.*).*",    # end of timing
        ],
        './',        # output directory of perf region tools
        'c'
    )



if len(sys.argv) > 1:
    if sys.argv[1] == 'preprocess':
        print("PREPROCESS")
        pf.preprocessor()

    elif sys.argv[1] == 'cleanup':
        print("CLEANUP")
        pf.cleanup()

    else:
        print("Unsupported argument "+sys.argv[1])

else:
    pf.preprocessor()
