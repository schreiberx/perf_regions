FC:=gfortran
CC:=gcc
CXX:=g++

PAPI_LIB:=-L/users/home/opt/papi/papi-5.1.0/lib/ -lpapi 
PERFREGION_LIB:=-L../../build -lperf_regions

LINKFLAGS_PERFREGION:=${PAPI_LIB} ${PERFREGION_LIB}

all:	fortran

fortran:
	./perf_regions_instrumentation.py cleanup
	${FC} -g -o nemo_test nemo_test.F90

fortran_perfregion:
	./perf_regions_instrumentation.py preprocess
	${FC} -g -o nemo_test nemo_test.F90 ${LINKFLAGS_PERFREGION} ${PAPI_LIB}

clean :
	./perf_regions_instrumentation.py cleanup
	rm -f nemo_test
	rm -f perf_region_list.txt
