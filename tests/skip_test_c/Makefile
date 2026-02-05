CC ?= gcc
include ../Makefile.setup

CFLAGS_PERFREGION+= -I../../src -DUSE_MPI=0
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

# Ensure build directory exists
$(shell mkdir -p build)

all: build/skip_test

build/skip_test: src/skip_test.c
	${CC} -g -std=c99 -o $@ $< ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
