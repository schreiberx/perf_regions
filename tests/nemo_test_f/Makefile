FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src -O0 -g
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB} 

all:	tests
	
tests:
	make -C ../../src
	make nemo_test
	make nemo_test_perf_regions

run_tests:
	./run_tests.sh

nemo_test:
	./perf_regions_instrumentation.py cleanup
	${FC} -g -o nemo_test nemo_test.F90

nemo_test_perf_regions: nemo_test.F90
	./perf_regions_instrumentation.py cleanup
	./perf_regions_instrumentation.py preprocess
	${FC} -g -I../../src -o nemo_test_perf_regions nemo_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f nemo_test nemo_test_perf_regions perf_regions_fortran.o *.mod
	rm -f perf_region_list.txt

.PHONY: all tests run_tests nemo_test nemo_test_perf_regions clean
