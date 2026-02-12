#! /bin/bash

sed -i "s/^USE_MPI=[01]/USE_MPI=0/" ../config_tests.mk
sed -i "s/^USE_PAPI=[01]/USE_PAPI=1/" ../config_tests.mk
sed -i "s/^USE_FORTRAN=[01]/USE_FORTRAN=0/" ../config_tests.mk

make

./run_bandwidth_monitor.py
