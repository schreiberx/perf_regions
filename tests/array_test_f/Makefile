include ../Makefile.common

all: $(TEST_BUILD_DIR)/array_test $(TEST_BUILD_DIR)/array_test_perf_regions

$(TEST_BUILD_DIR)/array_test: src/array_test.F90 | $(TEST_BUILD_DIR)
	$(FC) -O2 -o $@ $< $(FCFLAGS) $(LDFLAGS)

$(TEST_BUILD_DIR)/array_test.F90: src/array_test.F90 | $(TEST_BUILD_DIR)
	cp $< $@
	$(PYTHON) $(INSTRUMENT) preprocess $@

$(TEST_BUILD_DIR)/array_test_perf_regions: $(TEST_BUILD_DIR)/array_test.F90
	$(FC) -O2 -o $@ $< $(FCFLAGS) $(LDFLAGS) $(PERFREGION_FFLAGS) $(PERFREGION_LDFLAGS_FORT)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod

.PHONY: all clean
