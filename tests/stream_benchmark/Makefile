CFLAGS ?= -O2 -fopenmp

include ../Makefile.common

all: $(TEST_BUILD_DIR)/stream_c $(TEST_BUILD_DIR)/stream_c_perfregions

$(TEST_BUILD_DIR)/stream_c: src/stream.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ src/stream.c $(LDFLAGS)

$(TEST_BUILD_DIR)/stream.c: src/stream.c | $(TEST_BUILD_DIR)
	cp $< $@
	$(PYTHON) $(INSTRUMENT) preprocess $@

$(TEST_BUILD_DIR)/stream_c_perfregions: $(TEST_BUILD_DIR)/stream.c
	$(CC) $(CFLAGS) -DUSE_MPI=0 -o $@ $< $(LDFLAGS) $(PERFREGION_CFLAGS) $(PERFREGION_LDFLAGS)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod

.PHONY: all clean
