include ../Makefile.common

CFLAGS += -DUSE_MPI=0 $(PERFREGION_CFLAGS)
LDFLAGS += $(PERFREGION_LDFLAGS)

all: $(TEST_BUILD_DIR)/dynamic_regions

$(TEST_BUILD_DIR)/dynamic_regions: src/dynamic_regions.c | $(TEST_BUILD_DIR)
	$(CC) -O2 -std=c99 -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod

.PHONY: all clean
