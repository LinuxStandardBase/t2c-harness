#!/bin/sh

# Get the directory where this script resides and set T2C_ROOT.
TDIR="`dirname $0`/../"
WORK_DIR=$(cd ${TDIR} && pwd)
export T2C_ROOT=${WORK_DIR}

# Clean first, build later
${T2C_ROOT}/t2c/clean.sh

# Prepare the environment and build T2C.
if [ -z ${TET_ROOT} ]
then
        TET_ROOT=/opt/lsb-tet3-lite
fi

echo TET_ROOT is ${TET_ROOT}

TET_PATH=$TET_ROOT
PATH=/opt/lsb/bin:$PATH:$TET_PATH/bin
export TET_ROOT TET_PATH PATH

export CC=lsbcc

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
echo $CC_SPECIAL_FLAGS > $T2C_ROOT/t2c/cc_special_flags

# -------------------------------------------------------------------------
# Build T2C code generator.
cd ${T2C_ROOT}/t2c/src
make

if [ ! -x ${T2C_ROOT}/t2c/bin/t2c ]
then
        echo "Failed to build t2c, aborting..."
        exit 1
fi 

# Build a support library and special object files for standalone tests
cd ${T2C_ROOT}/t2c/debug/src
make

if [ ! -e ${T2C_ROOT}/t2c/debug/lib/dbgm.o ]
then
        echo "Failed to build dbgm.o, aborting..."
        exit 1
fi 

if [ ! -e ${T2C_ROOT}/t2c/debug/lib/t2c_util_d.a ]
then
        echo "Failed to build T2C support library (debug version), aborting..."
        exit 1
fi 

# Done
echo
echo Build completed successfully
echo "To use T2C code generator, you will probably need to set T2C_ROOT environment variable to ${T2C_ROOT}" 
