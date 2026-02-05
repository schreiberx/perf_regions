CC ?= gcc
include ../Makefile.setup

CFLAGS_PERFREGION:=-I../../src $(PAPI_INC)
LINKFLAGS_PERFREGION:=${PERFREGION_LIB} ${PAPI_LIB}

all: bandwidth_monitor_papi

bandwidth_monitor_papi: src/bandwidth_monitor_papi.c
	$(CC) -g -O2 $(CFLAGS_PERFREGION) -o bandwidth_monitor_papi src/bandwidth_monitor_papi.c $(LINKFLAGS_PERFREGION)

clean:
	rm -f bandwidth_monitor_papi
