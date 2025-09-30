#! /bin/bash


set -e

# make -C array_test_c run_tests USE_PAPI=${USE_PAPI}
# make -C array_test_f run_tests USE_PAPI=${USE_PAPI}
# make -C stream_benchmark run_tests USE_PAPI=${USE_PAPI}
# make -C nemo_test_f run_tests USE_PAPI=${USE_PAPI}
# make -C nested_test_f run_tests USE_PAPI=${USE_PAPI}

make -C array_test_c run_tests
make -C array_test_f run_tests
make -C stream_benchmark run_tests
make -C nemo_test_f run_tests
make -C nested_test_f run_tests

echo "ALL DONE"
