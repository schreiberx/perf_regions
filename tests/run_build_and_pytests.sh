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

# Check if MPI and PAPI are enabled in config.mk
if ! grep -q "^USE_MPI=1" ../config.mk; then
    echo "Error: MPI is not enabled in config.mk (USE_MPI=1 not found)."
    echo "Please ensure MPI is available and configured by running ./configure in the root directory."
    exit 1
fi

if ! grep -q "^USE_PAPI=1" ../config.mk; then
    echo "Error: PAPI is not enabled in config.mk (USE_PAPI=1 not found)."
    echo "Please ensure PAPI is available and configured by running ./configure in the root directory."
    exit 1
fi

cp ../config.mk ./config_tests.mk

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
    make -C .. USE_MPI=${1} USE_PAPI=${2} USE_FORTRAN=${3} > output_build_mpi_${1}_papi_${2}_fortran_${3}.txt 2>&1 || cat output_build_mpi_${1}_papi_${2}_fortran_${3}.txt
}

make_run_tests() {
    echo "Running tests with MPI=${1} PAPI=${2}"
    make_clean ${1} ${2} -1

    update_config ${1} ${2} 0
    make_build ${1} ${2} 0

    update_config ${1} ${2} 1
    make_build ${1} ${2} 1

    export USE_MPI=${1}
    export USE_PAPI=${2}
    python3 -m pytest -vvv # -s
}

make_run_tests 1 1
make_run_tests 1 0
make_run_tests 0 1
make_run_tests 0 0
