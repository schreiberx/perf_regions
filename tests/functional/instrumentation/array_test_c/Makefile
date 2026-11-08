include ../../../Makefile.common

CFLAGS += -DUSE_MPI=0 $(PERFREGION_CFLAGS)
LDFLAGS += $(PERFREGION_LDFLAGS)

all: $(TEST_BUILD_DIR)/array_test $(TEST_BUILD_DIR)/array_test_perf_regions

$(TEST_BUILD_DIR)/array_test: src/array_test.c | $(TEST_BUILD_DIR)
	$(CC) -O2 -std=c99 -o $@ $<

$(TEST_BUILD_DIR)/array_test.c: src/array_test.c | $(TEST_BUILD_DIR)
	cp $< $@
	$(PYTHON) $(INSTRUMENT) preprocess $@

$(TEST_BUILD_DIR)/array_test_perf_regions: $(TEST_BUILD_DIR)/array_test.c
	$(CC) -O2 -std=c99 -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod

.PHONY: all clean
