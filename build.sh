#!/usr/bin/env bash

set -x

SOURCE_DIR=`pwd`
BUILD_TYPE=${BUILD_TYPE:-release}
BUILD_DIR=${BUILD_DIR:-../build}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install}
COMPILE_DIR=${BUILD_DIR}/${BUILD_TYPE}-compile

mkdir -p $COMPILE_DIR \
&& cd $COMPILE_DIR \
&& cmake \
		-DCMAKE_BUILD_TYPE=$BUILD_TYPE \
		-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
		$SOURCE_DIR \
&& make $* \
&& make install
