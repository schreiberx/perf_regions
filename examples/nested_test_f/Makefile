FC:=gfortran
CC:=gcc
CXX:=g++

# PAPI settings
USE_PAPI ?= 1
ifeq ($(USE_PAPI),1)
PAPI_LIB:=-L. -lpapi
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

CFLAGS_PERFREGION:=-I../../src -O0 -g
LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

#all:	nested_test nested_test_perf_region
all:	build
	
build:
	make -C ../../src
	make nested_test_perf_regions


run_tests:	build
	./run_tests.sh

nested_test:
	./perf_regions_instrumentation.py cleanup
	${FC} -g -o nested_test nested_test.F90

nested_test_perf_regions: nested_test.F90
	./perf_regions_instrumentation.py preprocess
	${FC} -g -I../../src -o nested_test_perf_regions nested_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f nested_test nested_test_perf_regions perf_regions_fortran.o *.mod
	rm -f perf_region_list.txt

.PHONY: all build run_tests nested_test nested_test_perf_regions clean
