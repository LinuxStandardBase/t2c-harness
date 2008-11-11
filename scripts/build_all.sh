#!/bin/sh

if [ -z ${T2C_ROOT} ]
then
        echo "Error: T2C_ROOT is not defined."
        echo "The environment variable T2C_ROOT should contain a path to the main directory of T2C Framework."
        exit 1
fi

# if an argument is specified, it will be considered as regexp: only the subsuites 
# that match it will be built
if [ -z "$1" ]; then
    SUITE_EXPR="t2c"
else
    SUITE_EXPR="$1"
fi

# Get the directory where this script resides.
WORK_DIR=$(cd `dirname $0` && pwd) 
T2C_SUITE_ROOT=${WORK_DIR}
export T2C_SUITE_ROOT

# Default paths for LSB 
if [ -z ${LSB_ROOT} ]
then
    export LSB_ROOT=/opt/lsb
fi

if [ -z ${LSB_LIB_DIR} ]
then
    if [ -d "${LSB_ROOT}/lib64" ]; then
        LSB_LIB_DIR=${LSB_ROOT}/lib64
    else
        LSB_LIB_DIR=${LSB_ROOT}/lib
    fi
    export LSB_LIB_DIR
fi

echo "LSB_ROOT is ${LSB_ROOT}, LSB_LIB_DIR is ${LSB_LIB_DIR}"

#------------------------------------
cd ${T2C_SUITE_ROOT}

if [ -z ${TET_ROOT} ]
then
        TET_ROOT=/opt/lsb-tet3-lite
fi

echo TET_ROOT is ${TET_ROOT}

TET_PATH=$TET_ROOT
PATH=/opt/lsb/bin:$PATH:$TET_PATH/bin
export TET_ROOT TET_PATH PATH

if [ ! -x ${T2C_ROOT}/t2c/bin/t2c ]
then
        printf "t2c executable is not found, aborting...\n"
        exit 1
fi 

export CC=lsbcc
for x in /opt/lsb/lib*/pkgconfig; do
        PKG_CONFIG_PATH="${x}${PKG_CONFIG_PATH+:${PKG_CONFIG_PATH}}"
done
export PKG_CONFIG_PATH

# -------------------------------------------------------------------------
# [Temporary]
# If gcc is used, determine its version and adjust compiler flags accordingly.
# This is to circumvent the problem with GCC stack protection   
# on Ubuntu7.04(x86_64) etc. GCC older than v4.1.0 does not understand the
# -fno-stack-protector option.

GCC_VER=`(gcc -dumpversion) 2>/dev/null`
NEW_CC_FLAGS="-fno-stack-protector"
CC_SPECIAL_FLAGS=""

if [ "$GCC_VER" != "" ]; then
    # OK, found gcc. 
    # Strip prefixes first if there are any.
    GCC_VER=`echo $GCC_VER | LC_ALL=C sed 's/^[a-zA-Z]*\-//'`
    
    # Now determine major and minor version numbers.
    GCC_MAJOR_VER=`echo $GCC_VER | sed 's/^\([0-9]*\).*/\1/'`

    GCC_MINOR_VER=`echo $GCC_VER | sed 's/^\([0-9]*\)//'`
    GCC_MINOR_VER=`echo $GCC_MINOR_VER | sed 's/^\.\([0-9]*\).*/\1/'`
    GCC_MINOR_VER=${GCC_MINOR_VER:-0}
    
#   echo Major ver: $GCC_MAJOR_VER
#   echo Minor ver: $GCC_MINOR_VER
    
    if [ $GCC_MAJOR_VER -gt 4 ]; then

    CC_SPECIAL_FLAGS=$NEW_CC_FLAGS
    
    elif [ $GCC_MAJOR_VER -eq 4 ]; then

    if [ $GCC_MINOR_VER -ge 1  ]; then
        CC_SPECIAL_FLAGS=$NEW_CC_FLAGS
    fi

    fi

fi

export CC_SPECIAL_FLAGS
echo $CC_SPECIAL_FLAGS > $T2C_SUITE_ROOT/cc_special_flags

cd ${T2C_SUITE_ROOT}
export PATH=${T2C_ROOT}/t2c/bin:$PATH

# Remove the old TET scenario files.
rm -f ${T2C_SUITE_ROOT}/tet_scen
rm -f ${T2C_SUITE_ROOT}/real_tet_scen

# Remove the old list of subsuites that failed to build
rm -f ${T2C_SUITE_ROOT}/failed_to_build.list

echo "all" >> ${T2C_SUITE_ROOT}/real_tet_scen

# Build all test suites
for TEST_SUITE in *-t2c
do
    echo "$TEST_SUITE" | grep $SUITE_EXPR > /dev/null 2>&1
    if [ $? -eq 0 ]; then
    
        BUILD_FAILED=""
        if [ -d ${TEST_SUITE} ]
        then
            echo   "----------------------"
            printf "Building $TEST_SUITE\n"

            # Generate & build tests and test data for ${TEST_SUITE}
            cd ${T2C_SUITE_ROOT}
            if [ -d ${TEST_SUITE}/testdata/testdata_src ]
            then
                cd ${T2C_SUITE_ROOT}/${TEST_SUITE}/testdata/testdata_src
                make
                if [ $? -ne 0 ]
                then
                    printf "Failed to build test data\n"
                    BUILD_FAILED="yes"
                fi
            fi

            if [ -z "${BUILD_FAILED}" ]; then

                cd ${T2C_SUITE_ROOT}/${TEST_SUITE}
                chmod a+x gen_tests clean
                ./gen_tests

                if [ $? -ne 0 ]
                then
                    printf "Failed to generate tests for ${TEST_SUITE}, aborting...\n"
                    BUILD_FAILED="yes"
                fi 
                
                if [ -z "${BUILD_FAILED}" ]; then
                    cd ${T2C_SUITE_ROOT}/${TEST_SUITE}/tests
                    make
                    if [ $? -ne 0 ]
                    then
                        printf "Failed to build tests for ${TEST_SUITE}, aborting...\n"
                        BUILD_FAILED="yes"
                    fi
                fi
            fi

            cd ${T2C_SUITE_ROOT}
            if [ ! -z "${BUILD_FAILED}" ]; then
                echo "${TEST_SUITE}" >> ${T2C_SUITE_ROOT}/failed_to_build.list
            else
                # fill real_tet_scen
                echo "    :include:/${TEST_SUITE}/scenarios/func_scen" >> ${T2C_SUITE_ROOT}/real_tet_scen
            fi
        fi 
    fi
done

rm -f ${T2C_SUITE_ROOT}/tet_scen
mv ${T2C_SUITE_ROOT}/real_tet_scen ${T2C_SUITE_ROOT}/tet_scen

# Set permissions
chmod -R a+w ${T2C_SUITE_ROOT}
if [ $? -ne 0 ]
then
        printf "Failed to set permissions. Perhaps you should use root account.\n"
        exit 1
fi

# Done
echo
echo Build completed.

if [ -f "${T2C_SUITE_ROOT}/failed_to_build.list" ]; then
    echo The following subsuites failed to build:
    cat ${T2C_SUITE_ROOT}/failed_to_build.list
else
    echo All subsuites are built successfully.
fi

