#!/bin/sh
folder=release

#clean
echo ==========; echo CLEAN; echo ==========
make -C ../../core clean
make -C ../helpware clean
rm ../../core/libcore.a
rm ../helpware/helper
rm ../helpware/dfu
#rm ../helpware/animate
rm ../helpware/display
#rm ../helpware/commander
#rm -rf $folder

#rebuild helper applications
echo ==========; echo BUILD; echo ==========
make -C ../../core
#make -C ../helpware animate
make -C ../helpware dfu
make -C ../helpware helper
make -C ../helpware display
#make -C ../helpware commander

#collect files
echo ==========; echo COLLECT; echo ==========
mkdir -p $folder
#cp ../helpware/animate ./$folder
cp ../helpware/dfu ./$folder
cp ../helpware/helper ./$folder
cp ../helpware/display ./$folder
#cp ../helpware/commander ./$folder
cp ../emmcdl/headprogram ./$folder
cp ../emmcdl/makebc/makebc ./$folder

echo Done!
