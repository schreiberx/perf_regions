FC ?= gfortran
CC ?= gcc
CXX ?= g++
CFLAGS ?= -O2 -fopenmp

include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src -O0 -g -DUSE_MPI=0
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}


# Ensure build directory exists
$(shell mkdir -p build)

all: build/stream_c build/stream_c_perfregions

build/stream_c: src/stream.c
	$(CC) $(CFLAGS) -o $@ src/stream.c

build/stream.c: src/stream.c
	cp $< $@
	./bin/perf_regions_instrumentation.py preprocess

build/stream_c_perfregions: build/stream.c
	$(CC) $(CFLAGS) -o $@ build/stream.c ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
