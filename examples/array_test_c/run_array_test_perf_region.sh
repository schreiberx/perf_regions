#! /bin/bash

export LD_LIBRARY_PATH=../../build:$LD_LIBRARY_PATH
export LIST_COUNTERS=PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,PAPI_TOT_INS

taskset -c 0 ./array_test_perf_region
