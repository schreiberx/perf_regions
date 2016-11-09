FC:=gfortran
CC:=gcc
CXX:=g++

PAPI_LIB:=-L/users/home/opt/papi/papi-5.1.0/lib/ -lpapi 
PERFREGION_LIB:=-L../../build -lperf_regions

CFLAGS_PERFREGION:=-I../../src
LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

all:	array_test array_test_perf_region

array_test:
	./perf_regions_instrumentation.py cleanup
	${CC} -g -o array_test array_test.c

array_test_perf_region:
	./perf_regions_instrumentation.py preprocess
	${CC} -g -o array_test_perf_region array_test.c ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f array_test array_test_perf_region
	rm -f perf_region_list.txt
