SUBDIRS = \
	functional/instrumentation/array_test_c \
	functional/instrumentation/array_test_f \
	functional/instrumentation/nemo_test_f \
	functional/instrumentation/nemo_test_f_direct \
	functional/regions/skip_test_c \
	functional/regions/repeated_start_c \
	functional/regions/dynamic_regions_c \
	hardware/papi/bandwidth_monitor_papi \
	hardware/papi/perf_regions_counters \
	benchmarks/stream_benchmark

all: run_tests

run_tests:
	python3 run_tests.py --fast

test: run_tests

test_papi:
	$(MAKE) -C .. test_papi

test_hardware:
	$(MAKE) -C .. test_hardware

test_all:
	$(MAKE) -C .. test_all

test_papi_through_perf_regions:
	$(MAKE) -C .. test_papi_through_perf_regions

run_tests_all:
	./run_tests_all.sh

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f output_clean_*.txt output_build_*.txt

.PHONY: all run_tests test test_papi test_hardware test_all run_tests_all test_papi_through_perf_regions clean
