#!/bin/bash

#$HOME/projects/vic-toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -static -I$(pwd)/include -L$(pwd)/lib/pi -lpv_porcupine -lpthread -o build/pv_server server/pv_server.c

#$HOME/projects/vic-toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -I$(pwd)/include -L$(pwd)/lib/pi -o build/pv_server server/pv_server.c -lpthread -lm -lpv_porcupine


$HOME/projects/vic-toolchain/arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc -I$(pwd)/include -L$(pwd)/lib/pi -o build/pv_server server/pv_server.c -Wl,--dynamic-linker=/anki/lib/hf-fs/ld-linux-armhf.so.3 -lpv_porcupine -lpthread -lm


#$HOME/.anki/vicos-sdk/dist/1.1.0-r04/prebuilt/bin/arm-oe-linux-gnueabi-clang -static -I$(pwd)/include -L$(pwd)/lib/pi -lpv_porcupine -lpthread -o build/pv_server server/pv_server.c

$HOME/.anki/vicos-sdk/dist/1.1.0-r04/prebuilt/bin/arm-oe-linux-gnueabi-clang -c -fPIC -I$(pwd)/include -o build/pv_porcupine.o interface/pv_interface.c
$HOME/.anki/vicos-sdk/dist/1.1.0-r04/prebuilt/bin/arm-oe-linux-gnueabi-ar rcs build/libpv_porcupine_interface.a build/pv_porcupine.o
