CC ?= gcc

# Use OpenMP when the selected compiler can compile and link it. This avoids a
# hard dependency on libomp for compilers that do not bundle an OpenMP runtime.
STREAM_OPENMP_FLAGS := $(shell printf 'int main(void) { return 0; }\n' | \
	$(CC) -x c -fopenmp -o /dev/null - >/dev/null 2>&1 && echo -fopenmp)
CFLAGS ?= -O2 $(STREAM_OPENMP_FLAGS)

# Keep the upstream STREAM defaults unless a test overrides them.
STREAM_ARRAY_SIZE ?= 20000000
STREAM_NTIMES ?= 50
STREAM_DEFINES = -DSTREAM_ARRAY_SIZE=$(STREAM_ARRAY_SIZE) -DNTIMES=$(STREAM_NTIMES)

include ../../Makefile.common

all: $(TEST_BUILD_DIR)/stream_c $(TEST_BUILD_DIR)/stream_c_perfregions

$(TEST_BUILD_DIR)/stream_c: src/stream.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) $(STREAM_DEFINES) -o $@ src/stream.c $(LDFLAGS)

$(TEST_BUILD_DIR)/stream.c: src/stream.c | $(TEST_BUILD_DIR)
	cp $< $@
	$(PYTHON) $(INSTRUMENT) preprocess $@

$(TEST_BUILD_DIR)/stream_c_perfregions: $(TEST_BUILD_DIR)/stream.c
	$(CC) $(CFLAGS) $(STREAM_DEFINES) -DUSE_MPI=0 -o $@ $< $(LDFLAGS) $(PERFREGION_CFLAGS) $(PERFREGION_LDFLAGS)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod

.PHONY: all clean
