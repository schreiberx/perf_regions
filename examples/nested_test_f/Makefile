FC ?= gfortran
CC ?= gcc
CXX ?= g++


include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src -O0 -g
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB} 


all:	build
	
build:
	make -C ../../src
	make nested_test_perf_regions

run_tests:
	./run_tests.sh

nested_test:
	./perf_regions_instrumentation.py cleanup
	${FC} -g -o nested_test nested_test.F90

nested_test_perf_regions: nested_test.F90
	./perf_regions_instrumentation.py cleanup
	./perf_regions_instrumentation.py preprocess
	${FC} -g -I../../src -o nested_test_perf_regions build_perf_regions/nested_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}
	./perf_regions_instrumentation.py cleanup

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f nested_test nested_test_perf_regions perf_regions_fortran.o *.mod
	rm -f perf_region_list.txt

.PHONY: all build run_tests nested_test nested_test_perf_regions clean
