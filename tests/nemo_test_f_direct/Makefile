FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

# Ensure build directory exists
$(shell mkdir -p build)

all:	build/nemo_test_perf_regions

build/nemo_test_perf_regions: src/nemo_test.F90
	${FC} -g -I../../src -o $@ $< ${FCFLAGS} ${LDFLAGS} ${PERFREGION_FFLAGS} ${PERFREGION_LDFLAGS_FORT}

clean :
	rm -rf build *.mod perf_region_list.txt

.PHONY: all clean

