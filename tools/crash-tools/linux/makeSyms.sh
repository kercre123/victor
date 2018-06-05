#!/bin/bash

set -e

echo "Renaming *.full -> * in bin and lib folders"
pushd bin
rename 's/\.full$//' *.full
popd # bin
pushd lib
rename 's/\.so\.full$/\.so/' *.so.full
popd # bin

rm -rf symbols

echo "Making symbols for * in bin folder"
pushd bin
for i in `ls *`; do
    echo Processing ${i}...
    ./../dump_syms $i . > $i.sym || ./../dump_syms -v $i > $i.sym
    VER=`head -1 $i.sym | awk '{print $4;}'`
    mkdir -p ../symbols/$i/$VER
    mv $i.sym ../symbols/$i/$VER/
done
popd # bin

echo "Making symbols for *.so in lib folder"
pushd lib
for i in `ls *.so`; do
    echo Processing ${i}...
    ./../dump_syms $i . > $i.sym || ./../dump_syms -v $i > $i.sym
    VER=`head -1 $i.sym | awk '{print $4;}'`
    mkdir -p ../symbols/$i/$VER
    mv $i.sym ../symbols/$i/$VER/
done
popd # lib
