FC ?= gfortran
CC ?= gcc
CXX ?= g++
CFLAGS ?= -O2 -fopenmp

include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src -O0 -g
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}


all: build

build:
	make -C ../../src
	make stream_c

run_tests:
	./run_tests.py

stream_c: stream.c
	$(CC) $(CFLAGS) stream.c -o stream_c

stream_c_perfregions: stream.c
	./perf_regions_instrumentation.py
	$(CC) $(CFLAGS) build_perf_regions/stream.c -o stream_c_perfregions ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	$(RM) -r build_perf_regions
	$(RM) stream_f stream_c stream_c_perfregions  *.o

