#!/bin/sh

# Get the directory where this script resides and set T2C_ROOT.
TDIR="`dirname $0`/../"
WORK_DIR=$(cd ${TDIR} && pwd)
export T2C_ROOT=${WORK_DIR}

# Remove special compiler flag list
rm -rf ${T2C_ROOT}/t2c/cc_special_flags

# Clean T2C code generator.
cd ${T2C_ROOT}/t2c/src
make clean

cd ${T2C_ROOT}/t2c/debug/src
make clean
