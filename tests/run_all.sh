#! /bin/bash


set -e

make -C array_test_c run_tests
make -C array_test_f run_tests
make -C stream_benchmark run_tests
make -C nemo_test_f_direct run_tests
make -C nemo_test_f run_tests
make -C bandwidth_monitor_papi run_tests

echo "ALL DONE"
