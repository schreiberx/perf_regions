
PAPI_INC:=
PAPI_LIB:=

ifndef F90
	F90:=gfortran
endif

ifndef CFLAGS
	CFLAGS:=${CFLAGS} -g -O3 -fPIC -Wall
endif

ifndef USE_MPI
	USE_MPI:=1
endif

BUILD_DIR:=../build/

mode.:=-DPERF_DEBUG=0
mode.release:=-DPERF_DEBUG=0
mode.debug:=-DPERF_DEBUG=1

CFLAGS:=${CFLAGS} ${mode.${MODE}} -DUSE_MPI=${USE_MPI}
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
	${CC} -fPIC -shared ${PAPI_INC} ${PAPI_LIB} ${OBJS} -o ${TARGET_LIB}

${BUILD_DIR}/papi_counters.o: papi_counters.c papi_counters.h
	mkdir -p ${BUILD_DIR}
	${CC} ${CFLAGS} -c -g ${PAPI_INC} ${PAPI_LIB} -c papi_counters.c -o ${BUILD_DIR}/papi_counters.o

${BUILD_DIR}/%.o: %.c %.h
	mkdir -p ${BUILD_DIR}
	${CC} ${CFLAGS} -c -g $< -o $@

# Generate Fortran ugly interfaces
${BUILD_DIR}/perf_regions_fortran.o: perf_regions_fortran.F90
	mkdir -p ${BUILD_DIR}
	${F90} -cpp -c perf_regions_fortran.F90 -o ${BUILD_DIR}/perf_regions_fortran.o
	mv -f perf_regions_fortran.mod "${BUILD_DIR}"

# -include Makefile.deps

Makefile.deps:
	$(CC) $(CFLAGS) -MM *.c > $@
	$(F90) $(F90FLAGS) -MM *.F90 >> $@


clean:
	rm -f ${TARGET_LIB}
	rm -f ${OBJS}
	rm -f ${MODS}
