CC ?= gcc
include ../Makefile.setup

CFLAGS_PERFREGION+= -I../../src -DUSE_MPI=0
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

all: skip_test

skip_test: skip_test.c
	${CC} -g -std=c99 -o skip_test skip_test.c ${CFLAGS_PERFREGION} ${LINKFLAGS_PERFREGION}

clean:
	rm -f skip_test *.o *.mod
