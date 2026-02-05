FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

# Ensure build directory exists
$(shell mkdir -p build)

all:	build/nemo_test build/nemo_test_perf_regions

build/nemo_test: src/nemo_test.F90
	${FC} -g -I../../src -o $@ $<

build/nemo_test.F90: src/nemo_test.F90
	cp $< $@
	./bin/perf_regions_instrumentation.py preprocess

build/nemo_test_perf_regions: build/nemo_test.F90
	${FC} -g -I../../src -o $@ $< ${FCFLAGS} ${LDFLAGS} ${PERFREGION_FFLAGS} ${PERFREGION_LDFLAGS_FORT}

clean:
	rm -rf build *.o *.mod
	rm -f nemo_test nemo_test_perf_regions perf_regions_fortran.o *.mod
	rm -f perf_region_list.txt

.PHONY: all tests nemo_test nemo_test_perf_regions clean
