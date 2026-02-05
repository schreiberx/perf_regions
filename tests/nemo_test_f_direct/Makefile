FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

# Ensure build directory exists
$(shell mkdir -p build)

all:	build/nemo_test_perf_regions

build/nemo_test_perf_regions: src/nemo_test.F90
	${FC} -g -I../../src -J build -o $@ $< ${LINKFLAGS_PERFREGION}

clean :
	rm -rf build *.mod perf_region_list.txt

.PHONY: all clean

