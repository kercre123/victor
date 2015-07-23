#!/usr/bin/env bash
find . -name '*.h'    > c-source.txt
find . -name '*.hpp' >> c-source.txt
find . -name '*.c'   >> c-source.txt
find . -name '*.cpp' >> c-source.txt
