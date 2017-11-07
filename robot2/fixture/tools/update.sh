#!/bin/sh
folder=datalocalfixture

#clean
echo ==========; echo CLEAN; echo ==========
make -C ../../core clean
make -C ../helpware clean
rm ../../core/libcore.a
rm ../helpware/helper
rm ../helpware/dfu
rm ../helpware/animate
rm ../helpware/display
rm -rf $folder

#rebuild helper applications
echo ==========; echo BUILD; echo ==========
make -C ../../core
make -C ../helpware animate
make -C ../helpware dfu
make -C ../helpware helper
make -C ../helpware display

#collect files
echo ==========; echo COLLECT; echo ==========
mkdir $folder
cp ../helpware/animate ./$folder
cp ../helpware/dfu ./$folder
cp ../helpware/helper ./$folder
cp ../helpware/display ./$folder
cp headprogram ./$folder
cp helpify ./$folder

#XXX: fetch latest emmcdl folder
mkdir $folder/emcdl #XXX: manually copy files here
