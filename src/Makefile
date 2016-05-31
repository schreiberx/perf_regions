

PAPI_INC:=/users/home/opt/papi/papi-5.1.0/include/
PAPI_LIB:=/users/home/opt/papi/papi-5.1.0/lib/



#MPICC=mpiicc
MPICC:=gcc
BUILD_DIR:=../build/
CFLAGS:=-g -O3 -fPIC
#CFLAGS:=-g -O0 -fPIC



all: ${BUILD_DIR}/libperf_regions.so
#	ar rcs libpm.a papi_counters.o



${BUILD_DIR}/libperf_regions.so : ${BUILD_DIR}/papi_counters.o ${BUILD_DIR}/perf_region_names.o ${BUILD_DIR}/perf_regions.o
#	${MPICC} -I${PAPI_INC} -L${PAPI_LIB} -lpapi -c papi_counters.c -o papi_counters.o
	${MPICC} -fPIC -shared -I${PAPI_INC} -L${PAPI_LIB} ${BUILD_DIR}/papi_counters.o ${BUILD_DIR}/perf_regions.o ${BUILD_DIR}/perf_region_names.o -o ${BUILD_DIR}/libperf_regions.so -lpapi

${BUILD_DIR}/papi_counters.o: papi_counters.c papi_counters.h
	${MPICC} ${CFLAGS} -c -g -I${PAPI_INC} -L${PAPI_LIB} -c papi_counters.c -o ${BUILD_DIR}/papi_counters.o  -lpapi
	
${BUILD_DIR}/perf_region_names.o: perf_region_names.c perf_region_names.h
	${MPICC} ${CFLAGS} -c -g perf_region_names.c -o ${BUILD_DIR}/perf_region_names.o

${BUILD_DIR}/perf_regions.o: perf_regions.c
	${MPICC} ${CFLAGS} -c -g perf_regions.c -o ${BUILD_DIR}/perf_regions.o



clean:
	rm -f ${BUILD_DIR}/perf_regions.so
	rm -f ${BUILD_DIR}/papi_counters.o
	rm -f ${BUILD_DIR}/perf_region_names.o
	rm -f ${BUILD_DIR}/perf_regions.o
	rm -f ${BUILD_DIR}/libperf_regions.so
