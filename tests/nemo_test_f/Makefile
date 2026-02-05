FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS_PERFREGION+= -I../../src -O0 -g
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB} 

# Ensure build directory exists
$(shell mkdir -p build)

all:	build/nemo_test build/nemo_test_perf_regions

build/nemo_test: src/nemo_test.F90
	${FC} -g -I../../src -J build -o $@ $<

build/nemo_test.F90: src/nemo_test.F90
	./perf_regions_instrumentation.py preprocess

build/nemo_test_perf_regions: build/nemo_test.F90
	${FC} -g -I../../src -J build -o $@ $< ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
	rm -f nemo_test nemo_test_perf_regions perf_regions_fortran.o *.mod
	rm -f perf_region_list.txt

.PHONY: all tests run_tests nemo_test nemo_test_perf_regions clean
