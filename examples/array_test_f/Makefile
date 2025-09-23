FC:=gfortran
CC:=gcc
CXX:=g++

PAPI_LIB:=-L. -lpapi 

# Detect which build system to use
ifneq (,$(wildcard ../../build/libperf_regions.so))
    # CMake build exists
    PERFREGION_LIB:=-L../../build -lperf_regions -I../../build
else ifneq (,$(wildcard ../../src/libperf_regions.so))
    # Traditional Makefile build exists  
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
	${FC} -g -o array_test_perf_region array_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

run_tests:	build
	./run_tests.sh

#perf_regions_fortran.o:
#	${FC} -c perf_regions_fortran.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f array_test array_test_perf_region array_test_f_perf_region perf_regions_fortran.o *.mod
	rm -f perf_region_list.txt

.PHONY: all build array_test_perf_region run_tests clean
