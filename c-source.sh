#!/usr/bin/env bash
find . -name '*.h'     > c-source.txt
find . -name '*.hpp'  >> c-source.txt
find . -name '*.c'    >> c-source.txt
find . -name '*.cpp'  >> c-source.txt
find . -name '*.def'  >> c-source.txt
find . -name '*.clad' >> c-source.txt
