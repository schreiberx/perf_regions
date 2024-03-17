#! /bin/bash


set -e 

export LD_LIBRARY_PATH=../../build:$LD_LIBRARY_PATH
#export PERF_REGIONS_COUNTERS="PAPI_L3_DCM"
#export PERF_REGIONS_COUNTERS=""
export PERF_REGIONS_COUNTERS="PAPI_L3_TCM,PAPI_L2_TCM,WALLCLOCKTIME"

# Flops:
#PAPI_FP_OPS

CACHEBLOCK_SIZE=64


#if true; then
if false; then
	taskset -c 0 ./stream_c_perfregions.exe

else
	OUTPUT=$(taskset -c 0 ./stream_c_perfregions.exe)

	echo "$OUTPUT"
	echo
	echo "*****************************************************"
	echo "* PERF REGION OUTPUT *"
	echo "*****************************************************"

	echo -e "test\tMISS\tTIME\tBW"
	for i in COPY SCALE ADD TRIAD; do
		LINE="$(echo "$OUTPUT" | grep "$i")"
		AR=(${LINE})
		MISS=${AR[1]}
		TIME=${AR[4]}

		MISS=$(printf "%.14f" $MISS)
		TIME=$(printf "%.14f" $TIME)
		BW=$(echo "scale=14; ($MISS*$CACHEBLOCK_SIZE)/($TIME*1024.0*1024.0)" | bc)
		echo -e "$i\t$MISS\t$TIME\t$BW"
	done
fi
