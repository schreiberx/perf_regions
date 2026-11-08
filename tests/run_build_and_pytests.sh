#! /bin/bash

# Abort on error
set -e

# Install Python dependencies
pip install -r requirements.txt || source venv_setup.sh

# Handle configuration
if [ ! -f ../config.mk ]; then
    echo "config.mk not found. Running configure..."
    (cd .. && ./configure)
fi

# The test matrix can be run without MPI or PAPI; unavailable configurations
# are simply skipped.
HAVE_MPI=0
HAVE_PAPI=0
grep -q "^USE_MPI=1" ../config.mk && HAVE_MPI=1
grep -q "^USE_PAPI=1" ../config.mk && HAVE_PAPI=1

cp ../config.mk ./config_tests.mk

# If a test-local PAPI was activated with tests/tools/papi-local/env.sh, wire
# its paths into the generated test configuration.
LOCAL_PAPI_PREFIX="${PERF_REGIONS_TEST_PAPI_PREFIX:-${PAPI_PREFIX:-}}"
if [[ -n "$LOCAL_PAPI_PREFIX" && -f "$LOCAL_PAPI_PREFIX/.perf_regions_papi_install" ]]; then
    if [[ -f "$LOCAL_PAPI_PREFIX/lib64/libpapi.so" || -f "$LOCAL_PAPI_PREFIX/lib64/libpapi.a" ]]; then
        LOCAL_PAPI_LIBDIR="lib64"
    else
        LOCAL_PAPI_LIBDIR="lib"
    fi

    sed -i "s|^PAPI_CFLAGS=.*|PAPI_CFLAGS=-I$LOCAL_PAPI_PREFIX/include|" ./config_tests.mk
    sed -i "s|^PAPI_LDFLAGS=.*|PAPI_LDFLAGS=-L$LOCAL_PAPI_PREFIX/$LOCAL_PAPI_LIBDIR -lpapi|" ./config_tests.mk
    export PATH="$LOCAL_PAPI_PREFIX/bin:${PATH:-}"
    export LD_LIBRARY_PATH="$LOCAL_PAPI_PREFIX/$LOCAL_PAPI_LIBDIR:${LD_LIBRARY_PATH:-}"
    HAVE_PAPI=1
fi

update_config() {
    echo "========================================"
    echo "Compiling with MPI=$1 PAPI=$2" FORTRAN=${3}
    echo "========================================"

    sed -i "s/^USE_MPI=[01]/USE_MPI=${1}/" ./config_tests.mk
    sed -i "s/^USE_PAPI=[01]/USE_PAPI=${2}/" ./config_tests.mk
    sed -i "s/^USE_FORTRAN=[01]/USE_FORTRAN=${3}/" ./config_tests.mk
}

make_clean() {
    echo "Cleaning with MPI=${1} PAPI=${2} FORTRAN=${3}"
    make -C .. clean > output_clean_mpi_${1}_papi_${2}_fortran_${3}.txt 2>&1 || cat output_clean_mpi_${1}_papi_${2}_fortran_${3}.txt
}

make_build() {
    echo "Building with MPI=${1} PAPI=${2} FORTRAN=${3}"
    if ! make -C .. USE_MPI=${1} USE_PAPI=${2} USE_FORTRAN=${3} > output_build_mpi_${1}_papi_${2}_fortran_${3}.txt 2>&1; then
        cat output_build_mpi_${1}_papi_${2}_fortran_${3}.txt
        exit 1
    fi
}

make_run_tests() {
    echo "Running tests with MPI=${1} PAPI=${2}"
    make_clean ${1} ${2}

    update_config ${1} ${2} 0
    make_build ${1} ${2} 0

    update_config ${1} ${2} 1
    make_build ${1} ${2} 1

    export USE_MPI=${1}
    export USE_PAPI=${2}
    python3 -m pytest -vvv # -s
}

if [[ "$HAVE_MPI" == "1" && "$HAVE_PAPI" == "1" ]]; then
    make_run_tests 1 1
fi
if [[ "$HAVE_MPI" == "1" ]]; then
    make_run_tests 1 0
fi
if [[ "$HAVE_PAPI" == "1" ]]; then
    make_run_tests 0 1
fi
make_run_tests 0 0

rm ./config_tests.mk
