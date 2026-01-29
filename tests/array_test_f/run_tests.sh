#! /bin/bash

export LD_LIBRARY_PATH="../../build:$LD_LIBRARY_PATH"
export PERF_REGIONS_COUNTERS=""
export PERF_REGIONS_COUNTERS="PAPI_L1_TCM,PAPI_L2_TCM,PAPI_L3_TCM,WALLCLOCKTIME"


set -e

make clean
make tests

taskset -c 0 ./array_test
taskset -c 0 ./array_test_perf_regions

set +e
diff array_test.F90 array_test.F90_TEST_ORIG > /dev/null
if [ $? -eq 0 ]; then
	echo "This should have failed"
	exit 1
fi

set -e

diff array_test.F90 array_test.F90_TEST_PR
if [ $? -ne 0 ]; then
	echo "Check with PerfRegion source file failed"
	exit 1
fi

make clean

diff array_test.F90 array_test.F90_TEST_ORIG
if [ $? -ne 0 ]; then
	echo "Undoing in-situ changes failed"
	exit 1
fi

echo "==========================="
echo "Tests finished successfully"
echo "Tests finished successfully"
echo "Tests finished successfully"
echo "==========================="

