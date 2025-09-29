all:
	$(MAKE) -C array_test_c USE_PAPI=$(USE_PAPI)
	$(MAKE) -C array_test_f USE_PAPI=$(USE_PAPI)
	$(MAKE) -C stream_benchmark USE_PAPI=$(USE_PAPI)
	$(MAKE) -C nemo_test_f USE_PAPI=$(USE_PAPI)
	$(MAKE) -C nested_test_f USE_PAPI=$(USE_PAPI)

	$(MAKE) run_tests USE_PAPI=$(USE_PAPI)

run_tests:
	$(MAKE) -C array_test_c run_tests USE_PAPI=$(USE_PAPI)
	$(MAKE) -C array_test_f run_tests USE_PAPI=$(USE_PAPI)
	$(MAKE) -C stream_benchmark run_tests USE_PAPI=$(USE_PAPI)
	$(MAKE) -C nemo_test_f run_tests USE_PAPI=$(USE_PAPI)
	$(MAKE) -C nested_test_f run_tests USE_PAPI=$(USE_PAPI)


clean:
	$(MAKE) -C array_test_c clean
	$(MAKE) -C array_test_f clean
	$(MAKE) -C stream_benchmark clean
	$(MAKE) -C nemo_test_f clean
	$(MAKE) -C nested_test_f clean

.PHONY: all run_tests clean
