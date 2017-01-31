FC:=gfortran
CC:=gcc
CXX:=g++

PAPI_LIB:=-L. -lpapi 
PERFREGION_LIB:=-L../../build -lperf_regions -I../../build

CFLAGS_PERFREGION:=-I../../src -O0 -g
LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

all:	recursive_test recursive_test_perf_region

recursive_test:
	./perf_regions_instrumentation.py cleanup
	${FC} -g -o recursive_test recursive_test.F90

recursive_test_perf_region: recursive_test.F90
	./perf_regions_instrumentation.py preprocess
	${FC} -g -o recursive_test_perf_region recursive_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f recursive_test recursive_test_perf_region perf_regions_fortran.o
