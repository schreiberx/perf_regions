FC:=ftn
CC:=gcc
CXX:=g++

PAPI_LIB:=-L. -lpapi -h dynamic
PERFREGION_LIB:=-L../../build -lperf_regions

CFLAGS_PERFREGION:=-I../../src
LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

all:	array_test array_test_perf_region

array_test:
	./perf_regions_instrumentation.py cleanup
	${FC} -g -o array_test array_test.F90

array_test_perf_region: perf_regions_fortran.o array_test.F90
	./perf_regions_instrumentation.py preprocess
	${FC} -g -o array_test_perf_region perf_regions_fortran.o array_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

perf_regions_fortran.o:
	${FC} -c perf_regions_fortran.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	rm -f array_test array_test_perf_region perf_regions_fortran.o
