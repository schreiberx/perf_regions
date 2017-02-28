#! /bin/bash

export LD_LIBRARY_PATH=../../build:$LD_LIBRARY_PATH
#export LIST_COUNTERS="PAPI_L3_DCM"
export LIST_COUNTERS="PAPI_L3_TCM,PAPI_L2_TCM"

# Flops:
#PAPI_FP_OPS

CACHEBLOCK_SIZE=64

OUTPUT=$(taskset -c 0 ./stream_c_perfregions.exe)

echo "$OUTPUT"
echo
echo
echo
echo "*****************************************************"
echo "* PERF REGION OUTPUT *"
echo "*****************************************************"

echo -e "test\tMISS\tTIME\tBW"
for i in COPY SCALE ADD TRIAD; do
	MISS="$(echo "$OUTPUT" | grep "$i" | sed "s/$i\t//" | sed "s/\t.*//" | head -n 1)"
	TIME="$(echo "$OUTPUT" | grep "$i" | sed "s/$i\t//" | sed "s/\t.*//" | tail -n 1)"

	MISS=$(printf "%.14f" $MISS)
	TIME=$(printf "%.14f" $TIME)
	BW=$(echo "scale=14; ($MISS*$CACHEBLOCK_SIZE)/($TIME*1024.0*1024.0)" | bc)
	echo -e "$i\t$MISS\t$TIME\t$BW"
done
