#!/bin/bash


for i in 1 2 4 8 16 32 64 128; do
	export JPI=$i
	export JPJ=$i
	export JPK=72
	export LD_LIBRARY_PATH=../build:$LD_LIBRARY_PATH

	#
	# Use papi_avail command to check available performance counters
	# These are part of papi-tools package
	#
	export LIST_COUNTERS=FP_COMP_OPS_EXE,PAPI_L3_TCM
	export LIST_COUNTERS=PAPI_L3_TCM

	echo "***********************************"
	echo "SIZE: $i"
	echo "***********************************"
	taskset -c 0 ./nemo_test
	echo "***********************************"
done
