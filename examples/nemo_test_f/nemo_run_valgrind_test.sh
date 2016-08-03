#!/bin/bash

export JPI=150
export JPJ=100
export JPK=72

#
# Use papi_avail command to check available performance counters
# These are part of papi-tools package
#
export NUM_COUNTERS=2
export LIST_COUNTERS=FP_COMP_OPS_EXE,PAPI_L3_TCM

export NUM_COUNTERS=1
export LIST_COUNTERS=PAPI_L3_TCM

taskset -c 0 ./run_valgrind.sh ./nemo_test
