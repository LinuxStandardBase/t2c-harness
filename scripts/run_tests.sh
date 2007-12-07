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

if [ -z $1 ]
then
    tcc -e .
    
else
    #save old tet_scen
    mv -f tet_scen tet_scen.orig
    
    echo "all" >> tet_scen

    cat tet_scen.orig | grep $1 >> tet_scen
	if [ $? != 0 ] 
	then 
		echo Test suite is not found: $1.
	else
    	tcc -e . 
	fi
    
    #restore old tet_scen
    mv -f tet_scen.orig tet_scen
fi 

#remove temporaries
rm -rf tet_tmp_dir
