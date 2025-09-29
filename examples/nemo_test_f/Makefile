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

LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

all:	build

build:
	make -C ../../src
	make fortran_perfregion


run_tests:	build
	./run_tests.sh

#fortran:
#	./perf_regions_instrumentation.py cleanup
#	${FC} -g -o nemo_test nemo_test.F90

fortran_perfregion:
	./perf_regions_instrumentation.py preprocess
	${FC} -g -I../../src -o nemo_test_perf_regions nemo_test.F90 ${LINKFLAGS_PERFREGION} ${PAPI_LIB}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f nemo_test nemo_test_perf_regions *.mod
	rm -f perf_region_list.txt

.PHONY: all build run_tests fortran_perfregion clean
