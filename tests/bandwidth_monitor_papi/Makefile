CC ?= gcc
include ../Makefile.setup

CFLAGS += $(PERFREGION_CFLAGS)
LDFLAGS += $(PERFREGION_LDFLAGS)

all: bandwidth_monitor_papi

bandwidth_monitor_papi: src/bandwidth_monitor_papi.c
	$(CC) -g -O2 $(CFLAGS) -o bandwidth_monitor_papi src/bandwidth_monitor_papi.c $(LDFLAGS)

clean:
	rm -f bandwidth_monitor_papi
