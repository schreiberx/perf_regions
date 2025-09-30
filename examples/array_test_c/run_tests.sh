#! /bin/bash

export LD_LIBRARY_PATH=../../build:$LD_LIBRARY_PATH
export PERF_REGIONS_COUNTERS=
export PERF_REGIONS_COUNTERS=PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,PAPI_TOT_INS,WALLCLOCKTIME

set -e

make array_test_perf_region

taskset -c 0 ./array_test_perf_region
