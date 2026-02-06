CC ?= gcc
include ../Makefile.setup

CFLAGS += -DUSE_MPI=0 $(PERFREGION_CFLAGS)
LDFLAGS += $(PERFREGION_LDFLAGS)

# Ensure build directory exists
$(shell mkdir -p build)

all: build/skip_test

build/skip_test: src/skip_test.c
	${CC} -g -std=c99 -o $@ $< ${CFLAGS} ${LDFLAGS}

clean:
	rm -rf build *.o *.mod

.PHONY: all clean
