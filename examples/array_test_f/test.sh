#! /bin/bash

export LIST_COUNTERS="PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM"
export LD_LIBRARY_PATH="../../build:$LD_LIBRARY_PATH"

./array_test_perf_region
