

PAPI_INC:=/users/home/opt/papi/papi-5.1.0/include/
PAPI_LIB:=/users/home/opt/papi/papi-5.1.0/lib/


MPICC:=gcc
BUILD_DIR:=../build/
CFLAGS:=-g -O3 -fPIC -std=c99

mode.:=-DPERF_DEBUG=0
mode.release:=-DPERF_DEBUG=0
mode.debug:=-DPERF_DEBUG=1

# disable DEBUG overheads in perf region library
CFLAGS:=${CFLAGS} ${mode.${MODE}}


all: ${BUILD_DIR}/libperf_regions.so



${BUILD_DIR}/libperf_regions.so : ${BUILD_DIR}/papi_counters.o ${BUILD_DIR}/perf_region_names.o ${BUILD_DIR}/perf_regions.o ${BUILD_DIR}/perf_regions_fortran.o ${BUILD_DIR}/posix_clock.o
	mkdir -p ${BUILD_DIR}
#	${MPICC} -I${PAPI_INC} -L${PAPI_LIB} -lpapi -c papi_counters.c -o papi_counters.o
	${MPICC} -fPIC -shared -I${PAPI_INC} -L${PAPI_LIB} ${BUILD_DIR}/papi_counters.o ${BUILD_DIR}/perf_regions.o ${BUILD_DIR}/perf_region_names.o ${BUILD_DIR}/posix_clock.o -o ${BUILD_DIR}/libperf_regions.so -lpapi

${BUILD_DIR}/papi_counters.o: papi_counters.c papi_counters.h
	mkdir -p ${BUILD_DIR}
	${MPICC} ${CFLAGS} -c -g -I${PAPI_INC} -L${PAPI_LIB} -c papi_counters.c -o ${BUILD_DIR}/papi_counters.o  -lpapi

${BUILD_DIR}/%.o: %.c %.h
	mkdir -p ${BUILD_DIR}
	${MPICC} ${CFLAGS} -c -g $< -o $@

# Generate Fortran ugly interfaces
${BUILD_DIR}/perf_regions_fortran.o:
	mkdir -p ${BUILD_DIR}
	gfortran -c perf_regions_fortran.F90 -o ${BUILD_DIR}/perf_regions_fortran.o
	mv -f perf_regions_fortran.mod "${BUILD_DIR}"



clean:
	rm -f ${BUILD_DIR}/perf_regions.so
	rm -f ${BUILD_DIR}/papi_counters.o
	rm -f ${BUILD_DIR}/perf_region_names.o
	rm -f ${BUILD_DIR}/perf_regions.o
	rm -f ${BUILD_DIR}/libperf_regions.so
	rm -f ${BUILD_DIR}/perf_regions_fortran.o
