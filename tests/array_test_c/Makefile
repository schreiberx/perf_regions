FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

all:	tests


run_tests:
	./run_tests.sh

tests: array_test array_test_perf_regions

array_test: array_test.c
	./perf_regions_instrumentation.py cleanup
	${CC} -g -std=c99 -o array_test array_test.c

array_test_perf_regions: array_test.c
	./perf_regions_instrumentation.py cleanup
	./perf_regions_instrumentation.py preprocess
	${CC} -g -std=c99 -o array_test_perf_regions array_test.c ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	./perf_regions_instrumentation.py cleanup
	rm -f array_test array_test_perf_regions *.o *.mod

.PHONY: all build run_tests tests array_test array_test_perf_regions clean
