#! /bin/bash

cd `dirname $0`

echo "Packing libs.tar.bz2"
tar -j -c -v -f ./libs.tar.bz2 ./include ./lib

