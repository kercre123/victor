#!/bin/bash

#clean
rm -rf Keil_4
mkdir Keil_4

#convert projx file
python ./tools/convert_keil_uv5_to_uv4.py  Keil_5/prod_test_anki_cube.uvprojx  Keil_4/prod_test_anki_cube.uvproj

#copy options file
cp ./Keil_5/prod_test_anki_cube.uvoptx ./Keil_4/prod_test_anki_cube.uvopt
