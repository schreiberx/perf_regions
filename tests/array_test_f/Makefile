FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup


# Ensure build directory exists
$(shell mkdir -p build)

all:	build/array_test build/array_test_perf_regions

build/array_test: src/array_test.F90
	${FC} -g -o $@ $< ${FCFLAGS} ${LDFLAGS}

build/array_test.F90: src/array_test.F90
	cp $< $@
	./bin/perf_regions_instrumentation.py preprocess

build/array_test_perf_regions: build/array_test.F90
	${FC} -g -o $@ $< ${FCFLAGS} ${LDFLAGS} ${PERFREGION_FFLAGS} ${PERFREGION_LDFLAGS_FORT}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
