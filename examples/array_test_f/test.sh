#! /bin/bash

export LD_LIBRARY_PATH="../../build:$LD_LIBRARY_PATH"
export PERF_REGIONS_COUNTERS=""
#export PERF_REGIONS_COUNTERS="PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM"

./array_test_perf_region
