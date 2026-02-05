FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS_PERFREGION+= -I../../src -DUSE_MPI=0
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

# Ensure build directory exists
$(shell mkdir -p build)

all:	build/array_test build/array_test_perf_regions

build/array_test: src/array_test.c
	${CC} -g -std=c99 -o $@ $<

# The instrumentation script reads src/* and writes to build/*
build/array_test.c: src/array_test.c
	cp $< $@
	./bin/perf_regions_instrumentation.py preprocess

# Compile the instrumented source code from build/
build/array_test_perf_regions: build/array_test.c
	${CC} -g -std=c99 -o $@ $< ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
