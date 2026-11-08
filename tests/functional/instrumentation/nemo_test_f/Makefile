include ../../../Makefile.common

all: $(TEST_BUILD_DIR)/nemo_test $(TEST_BUILD_DIR)/nemo_test_perf_regions

$(TEST_BUILD_DIR)/nemo_test: src/nemo_test.F90 | $(TEST_BUILD_DIR)
	$(FC) -g -I$(REPO_ROOT)/src -o $@ $<

$(TEST_BUILD_DIR)/nemo_test.F90: src/nemo_test.F90 | $(TEST_BUILD_DIR)
	cp $< $@
	$(PYTHON) $(INSTRUMENT) preprocess $@

$(TEST_BUILD_DIR)/nemo_test_perf_regions: $(TEST_BUILD_DIR)/nemo_test.F90
	$(FC) -g -I$(REPO_ROOT)/src -o $@ $< $(FCFLAGS) $(LDFLAGS) $(PERFREGION_FFLAGS) $(PERFREGION_LDFLAGS_FORT)

clean:
	rm -rf $(TEST_BUILD_DIR) *.o *.mod
	rm -f nemo_test nemo_test_perf_regions perf_regions_fortran.o
	rm -f perf_region_list.txt

.PHONY: all clean
