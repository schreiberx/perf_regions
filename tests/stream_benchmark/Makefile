FC ?= gfortran
CC ?= gcc
CXX ?= g++
CFLAGS ?= -O2 -fopenmp

include ../Makefile.setup

# Ensure build directory exists
$(shell mkdir -p build)

all: build/stream_c build/stream_c_perfregions

build/stream_c: src/stream.c
	$(CC) $(CFLAGS) -o $@ src/stream.c ${LDFLAGS}

build/stream.c: src/stream.c
	cp $< $@
	./bin/perf_regions_instrumentation.py preprocess

build/stream_c_perfregions: build/stream.c
	$(CC) $(CFLAGS) -DUSE_MPI=0 -o $@ build/stream.c ${LDFLAGS} $(PERFREGION_CFLAGS) $(PERFREGION_LDFLAGS)

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
