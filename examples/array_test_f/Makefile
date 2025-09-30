FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

all:	build

build:
	make -C ../../src
	make array_test_perf_regions

array_test_perf_regions: array_test.F90
	./perf_regions_instrumentation.py cleanup
	./perf_regions_instrumentation.py preprocess
	${FC} -g -I../../src -o array_test_perf_regions build_perf_regions/array_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}
	./perf_regions_instrumentation.py cleanup

run_tests:
	./run_tests.sh

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f array_test array_test_perf_regions array_test_f_perf_regions perf_regions_fortran.o *.mod

.PHONY: all build array_test_perf_regions run_tests clean
