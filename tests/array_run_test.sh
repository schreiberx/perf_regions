#! /bin/bash

export LD_LIBRARY_PATH=../build:$LD_LIBRARY_PATH
export LIST_COUNTERS=PAPI_L3_LDM

./array_test
