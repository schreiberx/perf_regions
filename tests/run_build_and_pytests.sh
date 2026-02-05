#! /bin/bash

# Abort on error
set -e

# Install Python dependencies
pip install -r requirements.txt || source venv_setup.sh

# Handle configuration backup or creation
if [ -f ../config.mk ]; then
    cp ../config.mk ../config.mk.bak
    HAS_CONFIG_BACKUP=1
else
    echo "config.mk not found. Running configure..."
    (cd .. && ./configure)
    HAS_CONFIG_BACKUP=0
fi

restore_config() {
    if [ "$HAS_CONFIG_BACKUP" -eq 1 ]; then
        mv ../config.mk.bak ../config.mk
    else
        rm -f ../config.mk
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
