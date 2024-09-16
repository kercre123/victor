#!/bin/bash

# for x86_64

gcc -I$(pwd)/include -L$(pwd)/lib/x86_64 -lpv_porcupine -lpthread -o build/pv_server server/pv_server.c

gcc -shared -fPIC -I$(pwd)/include -o build/libpv_porcupine.so interface/pv_interface.c
