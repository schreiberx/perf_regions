FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

all:	tests

build:
	make  -C ../../src
	make tests

run_tests:
	./run_tests.sh

tests: array_test array_test_perf_region

array_test:
	./perf_regions_instrumentation.py cleanup
	${CC} -g -std=c99 -o array_test array_test.c

array_test_perf_region:
	./perf_regions_instrumentation.py cleanup
	./perf_regions_instrumentation.py preprocess
	${CC} -g -std=c99 -o array_test_perf_region build_perf_regions/array_test.c ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}
	./perf_regions_instrumentation.py cleanup

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f array_test array_test_perf_region *.o *.mod
	rm -f perf_region_list.txt

.PHONY: all build run_tests tests array_test array_test_perf_region clean
