#!/bin/sh

# T2C_ROOT is not necessary to run the tests

# Get the directory where this script resides.
WORK_DIR=$(cd `dirname $0` && pwd) 
T2C_SUITE_ROOT=${WORK_DIR}
export T2C_SUITE_ROOT

cd ${T2C_SUITE_ROOT}

if [ -z ${TET_ROOT} ]
then
    TET_ROOT=/opt/lsb-tet3-lite
fi

echo TET_ROOT is ${TET_ROOT}
#echo T2C_SUITE_ROOT is ${T2C_SUITE_ROOT}

export TET_ROOT
export TET_PATH=$TET_ROOT
export PATH=/opt/lsb/bin:$PATH:$TET_PATH/bin
export TET_SUITE_ROOT=`pwd`

export CC=lsbcc
for x in /opt/lsb/lib*/pkgconfig; do
    PKG_CONFIG_PATH="${x}${PKG_CONFIG_PATH+:${PKG_CONFIG_PATH}}"
done
export PKG_CONFIG_PATH

#remove previous results and temporaries
rm -rf results
rm -rf tet_tmp_dir

#run the tests
TSFNAME="tet_scen"

# a helper function
countTestGroups()
{
    # count the groups of tests to be executed (may be useful for the programs
    # that will be processing the execution log)
    NUM_TEST_GROUPS=0
    
    while read RAWLN
    do
        echo ${RAWLN} | grep ":include:.*-t2c\/.*scenarios\/" > /dev/null 
        if [ $? -eq 0 ]; then
            LINE_OK=`echo ${RAWLN} | LC_ALL=C sed 's/:include:[\/]*//'`
            
            if [ ! -f "${LINE_OK}" ]; then
                echo "File not found: ${PWD}/${LINE_OK}"
                exit 1
            fi
            
            NTG=`cat "${LINE_OK}" | grep "\/tests\/" | wc -l`
            NUM_TEST_GROUPS=`expr ${NUM_TEST_GROUPS} + ${NTG}`
        fi
    done < ${TSFNAME}

    echo "-------------------------------------"
    echo "Number of test groups to be executed: ${NUM_TEST_GROUPS}"
    echo "-------------------------------------"
} # end of countTestGroups()

if [ -z $1 ]
then
    countTestGroups
    tcc -e .
    
else
    #save old tet scenario file
    mv -f ${TSFNAME} ${TSFNAME}.orig
    
    echo "all" >> ${TSFNAME}

    cat ${TSFNAME}.orig | grep $1 >> ${TSFNAME}
	if [ $? != 0 ] 
	then 
		echo Test suite is not found: $1.
	else
        countTestGroups
                
        # run the tests
    	tcc -e . 
	fi
    
    #restore old tet scenario file
    mv -f ${TSFNAME}.orig ${TSFNAME}
fi 

#remove temporaries
rm -rf tet_tmp_dir

exit 0
