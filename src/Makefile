
PAPI_INC:=
PAPI_LIB:=

F90=gfortran


MPICC:=gcc
BUILD_DIR:=../build/
CFLAGS:=-g -O3 -fPIC -Wall

mode.:=-DPERF_DEBUG=0
mode.release:=-DPERF_DEBUG=0
mode.debug:=-DPERF_DEBUG=1

# Disable DEBUG overheads in perf region library
CFLAGS:=${CFLAGS} ${mode.${MODE}}
F90FLAGS:=${F90FLAGS} ${mode.${MODE}}

SRCS_C:=$(wildcard *.c)
SRCS_F90:=$(wildcard *.F90)
OBJS:=$(patsubst %.c,${BUILD_DIR}/%.o,$(SRCS_C)) $(patsubst %.F90,${BUILD_DIR}/%.o,$(SRCS_F90))
MODS:=$(patsubst %.F90,${BUILD_DIR}/%.mod,$(SRCS_F90))
TARGET_LIB:=${BUILD_DIR}/libperf_regions.so

all:
	make Makefile.deps
	make ${TARGET_LIB}


${BUILD_DIR}/libperf_regions.so: ${OBJS}
	${MPICC} -fPIC -shared -I${PAPI_INC} -L${PAPI_LIB} ${OBJS} -o ${TARGET_LIB} -lpapi

${BUILD_DIR}/papi_counters.o: papi_counters.c papi_counters.h
	mkdir -p ${BUILD_DIR}
	${MPICC} ${CFLAGS} -c -g -I${PAPI_INC} -L${PAPI_LIB} -c papi_counters.c -o ${BUILD_DIR}/papi_counters.o  -lpapi

${BUILD_DIR}/%.o: %.c %.h
	mkdir -p ${BUILD_DIR}
	${MPICC} ${CFLAGS} -c -g $< -o $@

# Generate Fortran ugly interfaces
${BUILD_DIR}/perf_regions_fortran.o: perf_regions_fortran.F90
	mkdir -p ${BUILD_DIR}
	${F90} -cpp -c perf_regions_fortran.F90 -o ${BUILD_DIR}/perf_regions_fortran.o
	mv -f perf_regions_fortran.mod "${BUILD_DIR}"

-include Makefile.deps

Makefile.deps:
	$(CC) $(CFLAGS) -MM *.c > $@
	$(F90) $(F90FLAGS) -MM *.F90 >> $@


clean:
	rm -f ${TARGET_LIB}
	rm -f ${OBJS}
	rm -f ${MODS}
