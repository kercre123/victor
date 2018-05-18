#!/bin/sh
folder=release

#clean
echo ==========; echo CLEAN; echo ==========
make -C ../../core clean
make -C ../helpware clean
rm ../../core/libcore.a
rm ../helpware/helper
rm ../helpware/dfu
rm ../helpware/display

#rebuild helper applications
echo ==========; echo BUILD; echo ==========
make -C ../../core
make -C ../helpware dfu
make -C ../helpware helper
make -C ../helpware display

#collect files
echo ==========; echo COLLECT; echo ==========
mkdir -p $folder
cp ../helpware/dfu ./$folder
cp ../helpware/helper ./$folder
cp ../helpware/display ./$folder
cp ../emmcdl/makebc/makebc ./$folder

echo Done!
