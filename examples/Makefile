all:
	make -C array_test_c
	make -C array_test_f
	make -C stream_benchmark
	make -C nemo_test_f
	make -C nested_test_f

	make run_tests

run_tests:
	make -C array_test_c run_tests
	make -C array_test_f run_tests
	make -C stream_benchmark run_tests
	make -C nemo_test_f run_tests
	make -C nested_test_f run_tests


clean:
	make -C array_test_c clean
	make -C array_test_f clean
	make -C stream_benchmark clean
	make -C nemo_test_f clean
	make -C nested_test_f clean

.PHONY: all run_tests clean
