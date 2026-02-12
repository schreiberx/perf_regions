FC ?= gfortran
CC ?= gcc
CXX ?= g++

include ../Makefile.setup

CFLAGS+= -DUSE_MPI=0 $(PERFREGION_CFLAGS)
LDFLAGS+= $(PERFREGION_LDFLAGS)

# Ensure build directory exists
$(shell mkdir -p build)

all:	build/array_test build/array_test_perf_regions

build/array_test: src/array_test.c
	${CC} -O2 -std=c99 -o $@ $<

# The instrumentation script reads src/* and writes to build/*
build/array_test.c: src/array_test.c
	cp $< $@
	./bin/perf_regions_instrumentation.py preprocess

# Compile the instrumented source code from build/
build/array_test_perf_regions: build/array_test.c
	echo $(PAPI_LDFLAGS)
	${CC} -O2 -std=c99 -o $@ $< ${CFLAGS} ${LDFLAGS}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
