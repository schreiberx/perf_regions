FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS_PERFREGION+= -I../../src
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

# Ensure build directory exists
$(shell mkdir -p build)

all:	build/array_test build/array_test_perf_regions

build/array_test: src/array_test.F90
	${FC} -g -I../../src -J build -o $@ $< ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

build/array_test.F90: src/array_test.F90
	cp $< $@
	./bin/perf_regions_instrumentation.py preprocess

build/array_test_perf_regions: build/array_test.F90
	${FC} -g -I../../src -J build -o $@ $< ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
