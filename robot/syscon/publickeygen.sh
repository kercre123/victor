#!/bin/bash

#Breif: generates build/publickeys.h from bash
rm -f build/publickeys.h
python3 ./tools/export.py build/publickeys.h
