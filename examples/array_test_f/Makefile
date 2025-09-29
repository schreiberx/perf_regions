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

CFLAGS_PERFREGION:=-I../../src
LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

all:	build

build:
	make -C ../../src
	make array_test_perf_region

array_test_perf_region: array_test.F90
	./perf_regions_instrumentation.py cleanup
	./perf_regions_instrumentation.py preprocess
	${FC} -g -I../../src -o array_test_perf_region array_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

run_tests:	build
	./run_tests.sh

#perf_regions_fortran.o:
#	${FC} -c perf_regions_fortran.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f array_test array_test_perf_region array_test_f_perf_region perf_regions_fortran.o *.mod
	rm -f perf_region_list.txt

.PHONY: all build array_test_perf_region run_tests clean
