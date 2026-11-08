# Generic build rules for simple C test programs.

_C_COMMON := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))Makefile.common)
include $(_C_COMMON)

CFLAGS ?= -O2 -std=c99

TEST ?= $(notdir $(CURDIR))
TEST_SOURCE ?= src/$(TEST).c
TEST_CFLAGS ?= -DUSE_MPI=0 -std=c99
TEST_LDFLAGS ?=

all: $(TEST_BUILD_DIR)/$(TEST)

$(TEST_BUILD_DIR)/$(TEST): $(TEST_SOURCE) | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_CFLAGS) -o $@ $< \
		$(LDFLAGS) $(TEST_LDFLAGS) $(PERFREGION_CFLAGS) $(PERFREGION_LDFLAGS)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod

.PHONY: all clean
