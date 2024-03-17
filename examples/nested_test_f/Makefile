FC:=gfortran
CC:=gcc
CXX:=g++

PAPI_LIB:=-L. -lpapi 
PERFREGION_LIB:=-L../../build -lperf_regions -I../../build

CFLAGS_PERFREGION:=-I../../src -O0 -g
LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

#all:	nested_test nested_test_perf_region
all:	build
	
build:
	make -C ../../src
	make nested_test_perf_regions


run_tests:	build
	./run_tests.sh

nested_test:
	./perf_regions_instrumentation.py cleanup
	${FC} -g -o nested_test nested_test.F90

nested_test_perf_regions: nested_test.F90
	./perf_regions_instrumentation.py preprocess
	${FC} -g -o nested_test_perf_regions nested_test.F90 ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f nested_test nested_test_perf_regions perf_regions_fortran.o
