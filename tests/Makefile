SUBDIRS = \
	array_test_c \
	array_test_f \
	stream_benchmark \
	nemo_test_f_direct \
	nemo_test_f \
	bandwidth_monitor_papi \
	skip_test_c \
	repeated_start_c \
	dynamic_regions_c

all: run_tests_all

run_tests:
	python3 run_tests.py

test_papi_through_perf_regions:
	$(MAKE) -C .. test_papi_through_perf_regions

run_tests_all:
	./run_tests_all.sh

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f output_clean_*.txt output_build_*.txt

.PHONY: all run_tests run_tests_all test_papi_through_perf_regions clean
