#!/bin/bash

# People probably want to compile it using intel CC
# (This message is exalab-specific as it pertains to the module system on those installations.)
echo "Please make sure you loaded the compiler module of your choice."
echo "(e.g. module load intel_icc-14)"
echo "Press return to continue, ^C to return to your shell."
read -p "..."

#
# Script invocation logic: supply two parameters
# parameter 1: 'mic' or 'host'
# parameter 2: installation directory
#

HOST_ARG=host
MIC_ARG=mic

if [ "$#" -ne 2 ]; then
	echo need 2 arguments: 'host' or 'mic' followed by installation path
	exit 1
fi

if [ "$1" = "$HOST_ARG" ]; then
	MIC_BUILD=false
	PAPI_INSTALL_DIR="$2"
else
	if [ "$1" = "$MIC_ARG" ]; then
		MIC_BUILD=true
		PAPI_INSTALL_DIR="$2"
	else
		echo first argument must be 'host' or 'mic'
		exit 1
	fi
fi

# check for absolute path
case $PAPI_INSTALL_DIR in
	/*)
		;;
	*)
		PAPI_INSTALL_DIR="$PWD/$PAPI_INSTALL_DIR"
		;;
esac

#
# Fetch and extract PAPI sources (if needed), and change directory to PAPI source tree
#

PAPI_DIR=papi-5.3.0
PAPI_TAR=$PAPI_DIR.tar.gz
PAPI_SRC=http://icl.cs.utk.edu/projects/papi/downloads/$PAPI_TAR

if [ ! -f $PAPI_TAR ]; then
	curl -O $PAPI_SRC
fi

if [ ! -d $PAPI_DIR ]; then
	tar xvf $PAPI_TAR
fi

cd "$PAPI_DIR/src"

#
# Run configure according to script parameters, build and install
#

PAPI_CONF="--prefix=$PAPI_INSTALL_DIR --with-static-lib=yes --with-shared-lib=yes"

if $MIC_BUILD; then
	export I_MPI_CC="icc -mmic"
	PAPI_CONF="$PAPI_CONF --with-mic --host=x86_64-k1om-linux --with-arch=k1om"
else
	export I_MPI_CC="icc"
	PAPI_CONF="$PAPI_CONF --with-perf-events"
fi

echo "./configure $PAPI_CONF"
echo "If the above seems to be correct: press return, otherwise ^C to quit."
read -p "..."

# make sure no previous build leftovers can disrupt the compilation process
make clean

./configure $PAPI_CONF &&
	make -j &&
	make install &&
	echo "--------------------------------------------------------------------------------" &&
	echo "Done! PAPI installed in $PAPI_INSTALL_DIR" &&
	echo "--------------------------------------------------------------------------------"
