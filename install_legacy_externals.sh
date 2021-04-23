#!/bin/bash

DEPS_PATH=${DEPS_PATH:-~/src/victor-packages}

[ -d "./EXTERNALS" ] && rm -fr EXTERNALS

mkdir -p ./EXTERNALS/coretech_external

pushd EXTERNALS
tar zxvf ${DEPS_PATH}/anki-thirdparty-vector-163.tar.gz
tar zxvf ${DEPS_PATH}/vector-audio-assets-17.tar.gz
mv audio-assets victor-audio-assets
tar zxvf ${DEPS_PATH}/victor-animation-assets-2645.tar.gz
cd coretech_external
tar zxvf ${DEPS_PATH}/Cte_192.tar.gz

	  
