#!/bin/sh

# Get the directory where this script resides.
WORK_DIR=$(cd `dirname $0` && pwd) 
T2C_SUITE_ROOT=${WORK_DIR}
export T2C_SUITE_ROOT

# Remove special compiler flag list
rm -rf ${T2C_SUITE_ROOT}/cc_special_flags

# Remove old TET files.
rm -rf ${T2C_SUITE_ROOT}/tet_scen ${T2C_SUITE_ROOT}/tet_tmp_dir

cd ${T2C_SUITE_ROOT}

# Clean all test suites
for TEST_SUITE in *-t2c
do
    if [ -d ${TEST_SUITE} ]
    then
        echo   "----------------------"
        printf "Cleaning $TEST_SUITE\n"
        
        cd ${T2C_SUITE_ROOT}/$TEST_SUITE
        ./clean
        cd ${T2C_SUITE_ROOT}

        # Clean previously built test data
        if [ -d ${TEST_SUITE}/testdata/testdata_src ]
        then
             cd ${T2C_SUITE_ROOT}/${TEST_SUITE}/testdata/testdata_src
             make clean
             cd ${T2C_SUITE_ROOT}
        fi
    fi
done

