SUBDIRS = array_test_c array_test_f stream_benchmark nemo_test_f_direct nemo_test_f bandwidth_monitor_papi skip_test_c

all: run_tests

run_tests:
	python3 -m pytest

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
	rm -f output_clean_*.txt
	rm -f output_build_*.txt

.PHONY: all run_tests clean
