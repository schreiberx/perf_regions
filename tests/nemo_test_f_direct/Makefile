FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

all:	build

build:
	make -C ../../src
	make fortran_perfregion

run_tests:
	./run_tests.sh

fortran_perfregion:
	${FC} -g -I../../src -o nemo_test_perf_regions nemo_test.F90 ${LINKFLAGS_PERFREGION} ${PAPI_LIB}

clean :
	rm -f nemo_test nemo_test_perf_regions *.mod
	rm -f perf_region_list.txt

.PHONY: all build run_tests fortran_perfregion clean

