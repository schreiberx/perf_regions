#! /bin/bash

# Abort on error
set -e

# Install Python dependencies
pip install -r requirements.txt || source venv_setup.sh

# Backup configuration
cp ../config.mk ../config.mk.bak

restore_config() {
    if [ -f ../config.mk.bak ]; then
        mv ../config.mk.bak ../config.mk
    fi
}
trap restore_config EXIT

update_config() {
    local mpi=$1
    local papi=$2
    sed -i "s/^USE_MPI=.*/USE_MPI=$mpi/" ../config.mk
    sed -i "s/^USE_PAPI=.*/USE_PAPI=$papi/" ../config.mk
}

# Compile with MPI and with PAPI
echo "Compiling with MPI=1 PAPI=1"
update_config 1 1
make -C .. clean
make -C .. USE_MPI=1 USE_PAPI=1
echo "Artifacts after build 1:"
ls -l ../build/
export USE_MPI=1
export USE_PAPI=1
python3 -m pytest

# Compile with MPI and without PAPI
echo "Compiling with MPI=1 PAPI=0"
update_config 1 0
make -C .. clean
make -C .. USE_MPI=1 USE_PAPI=0
export USE_MPI=1
export USE_PAPI=0
python3 -m pytest

# Compile without MPI and with PAPI
echo "Compiling with MPI=0 PAPI=1"
update_config 0 1
make -C .. clean
make -C .. USE_MPI=0 USE_PAPI=1
export USE_MPI=0
export USE_PAPI=1
python3 -m pytest

# Compile without MPI and without PAPI
echo "Compiling with MPI=0 PAPI=0"
update_config 0 0
make -C .. clean
make -C .. USE_MPI=0 USE_PAPI=0
export USE_MPI=0
export USE_PAPI=0
python3 -m pytest
