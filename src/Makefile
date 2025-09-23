
PAPI_INC:=
PAPI_LIB:=

ifndef F90
	F90:=gfortran
endif

ifndef CFLAGS
	CFLAGS:=${CFLAGS} -g -O3 -fPIC -Wall
endif

ifeq ($(PERF_REGIONS_USE_PAPI),1)
	CFLAGS:=${CFLAGS} -DPERF_REGIONS_USE_PAPI=1
else
	CFLAGS:=${CFLAGS} -DPERF_REGIONS_USE_PAPI=0
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

# Install targets
PREFIX ?= /usr/local
DESTDIR ?=
INSTALL_LIBDIR ?= ${PREFIX}/lib
INSTALL_INCLUDEDIR ?= ${PREFIX}/include/perf_regions

install: ${TARGET_LIB}
	@echo "Installing to prefix: ${PREFIX}"
	@echo "DESTDIR: ${DESTDIR}"
	install -d ${DESTDIR}${INSTALL_LIBDIR}
	install -d ${DESTDIR}${INSTALL_INCLUDEDIR}
	install -m 644 ${TARGET_LIB} ${DESTDIR}${INSTALL_LIBDIR}/
	install -m 644 perf_regions.h ${DESTDIR}${INSTALL_INCLUDEDIR}/
	install -m 644 perf_regions_defines.h ${DESTDIR}${INSTALL_INCLUDEDIR}/
	@if [ -f ${BUILD_DIR}/perf_regions_fortran.mod ]; then \
		echo "Installing Fortran module..."; \
		install -m 644 ${BUILD_DIR}/perf_regions_fortran.mod ${DESTDIR}${INSTALL_INCLUDEDIR}/; \
	else \
		echo "Warning: Fortran module not found (build first)"; \
	fi
	@echo "Installation complete!"

uninstall:
	rm -f ${DESTDIR}${INSTALL_LIBDIR}/libperf_regions.so*
	rm -f ${DESTDIR}${INSTALL_INCLUDEDIR}/perf_regions.h
	rm -f ${DESTDIR}${INSTALL_INCLUDEDIR}/perf_regions_defines.h
	rm -f ${DESTDIR}${INSTALL_INCLUDEDIR}/perf_regions_fortran.mod
	@echo "Uninstallation complete!"
