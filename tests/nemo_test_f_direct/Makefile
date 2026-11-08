include ../Makefile.common

all: $(TEST_BUILD_DIR)/nemo_test_perf_regions

$(TEST_BUILD_DIR)/nemo_test_perf_regions: src/nemo_test.F90 | $(TEST_BUILD_DIR)
	$(FC) -g -I../../src -o $@ $< $(FCFLAGS) $(LDFLAGS) $(PERFREGION_FFLAGS) $(PERFREGION_LDFLAGS_FORT)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod perf_region_list.txt

.PHONY: all clean
