FC:=gfortran
CC:=gcc
CXX:=g++
PAPI_LIB:=/users/home/opt/papi/papi-5.1.0/lib/
COUNTERS_LIB:=../build
COUNTERS_INC:=../src

all:	c fortran

c:
	${CC} -g -L${PAPI_LIB} -L${COUNTERS_LIB}  -I${COUNTERS_INC} -o array_test array_test.c -lpapi -lperf_regions

fortran:
	${FC} -g -L${PAPI_LIB} -L${COUNTERS_LIB} -o nemo_test nemo_test.F90 -lpapi -lperf_regions

clean :
	rm -f nemo_test
	rm -f array_test
