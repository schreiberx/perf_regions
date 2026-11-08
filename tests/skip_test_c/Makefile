include ../Makefile.common

CFLAGS += -DUSE_MPI=0 $(PERFREGION_CFLAGS)
LDFLAGS += $(PERFREGION_LDFLAGS)

all: $(TEST_BUILD_DIR)/skip_test

$(TEST_BUILD_DIR)/skip_test: src/skip_test.c | $(TEST_BUILD_DIR)
	$(CC) -g -std=c99 -o $@ $< $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod

.PHONY: all clean
