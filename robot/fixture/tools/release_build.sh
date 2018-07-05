#!/bin/sh
folder=release

#clean
echo ==========; echo CLEAN; echo ==========
make -C ../../core clean
make -C ../helpware clean
rm ../../core/libcore.a
rm ../helpware/helper
rm ../helpware/dfu

#rebuild helper applications
echo ==========; echo BUILD; echo ==========
make -C ../../core
make -C ../helpware dfu
make -C ../helpware helper

#collect files
echo ==========; echo COLLECT; echo ==========
mkdir -p $folder
cp ../helpware/dfu ./$folder
cp ../helpware/helper ./$folder
cp ../emmcdl/makebc/makebc ./$folder

echo Done!
