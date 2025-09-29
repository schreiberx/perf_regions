FC:=gfortran
CC:=gcc
CXX:=g++

# PAPI settings
USE_PAPI ?= 1
ifeq ($(USE_PAPI),1)
PAPI_LIB:=-L/users/home/opt/papi/papi-5.1.0/lib/ -lpapi
else
PAPI_LIB:=
endif 

# Use traditional Makefile build (CMake support removed)
# Check if shared library exists, otherwise use static
ifneq (,$(wildcard ../../src/libperf_regions.so))
    PERFREGION_LIB:=-L../../src -lperf_regions
else ifneq (,$(wildcard ../../src/libperf_regions.a))
    PERFREGION_LIB:=-L../../src -lperf_regions
else
    # Default to src location
    PERFREGION_LIB:=-L../../src -lperf_regions
endif

CFLAGS_PERFREGION:=-I../../src
LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

all:	tests

build:
	make  -C ../../src
	make tests
	#make run_tests

run_tests: all
	./run_tests.sh

tests: array_test array_test_perf_region

array_test:
	./perf_regions_instrumentation.py cleanup
	${CC} -g -std=c99 -o array_test array_test.c

array_test_perf_region:
	./perf_regions_instrumentation.py preprocess
	${CC} -g -std=c99 -o array_test_perf_region array_test.c ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f array_test array_test_perf_region *.o *.mod
	rm -f perf_region_list.txt

.PHONY: all build run_tests tests array_test array_test_perf_region clean
