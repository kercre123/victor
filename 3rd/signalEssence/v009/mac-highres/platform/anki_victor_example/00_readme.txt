2017-04-07

MMFX Example
============
The source code in this directory builds "simple," which
demonstrates how to get audio into and out of MMFx.

The inputs for Sin (mic inputs) and Refin (reference input)
are specified in config.json.

The echo cancelled output appears as out/simple_sout.wav.

The example also demonstrates the use of the sediag diagnostic
interface.

Example Use
===========
$ make -C build config=releasedll

$ make simple

$ ./simple 
ReadConfigFile:  reading (config.json).......done
sin: trying to open (./stutter_white_noise_48khz.wav)...success
refin: trying to open (./stutter_white_noise_48khz.wav)...success
ReadConfigFile:  reading (config.json).......done
rout: trying to open (out/simple_rout.wav)...success
sout: trying to open (out/simple_sout.wav)...success
.........................
query sediagdiag name: (fdsearch_num_beams_to_search) = 36
type: 5 (where 5 == SE_DIAG_UINT16)
readWrite: 1 (where 1 == SE_DIAG_R)
numElements: 1
description: Number of beams we're searching in

(go listen to out/simple_sout.wav)

Example Use (Android Build)
===========================
The android build ships with static libraries

   $ touch ../../se_lib_public/mmfxpub.h

   $ make -C build -f mmif.make  verbose=1
   mmif_proj.c
   arm-linux-androideabi-clang -MMD -MP -DNDEBUG -DUSE_48K_ROUT_EQ
   -DSE_ASSERTION_SEGV -I../../../se_lib_public
   -I../../../project/anki_victor -I../../../se_lib_public/cpu_none
   -I../../../thirdparty/sndfile/android-api24/include
   -I../../../thirdparty/CuTest/cutest-1.4
   -I../../../thirdparty/cJSON/cJSON.091209 -g -Wall -Wextra -g -O3
   -Werror -Wall -Werror=implicit-function-declaration
   -Wdeclaration-after-statement -Werror -mfpu=neon -msoft-float
   -ffast-math -march=armv7-a -mfloat-abi=softfp -mtune=cortex-a9
   -Wno-unused-command-line-argument -fPIE -no-integrated-as -o
   "obj/release/mmif/mmif_proj.o" -MF obj/release/mmif/mmif_proj.d -c
   "../../../project/anki_victor/mmif_proj.c"

   Linking mmif
   arm-linux-androideabi-ar -rcs ./libmmif.a obj/release/mmif/crossoverconfig.o
   obj/release/mmif/fdsearchconfig.o obj/release/mmif/spatialfilterconfig.o obj/release/mmif/mmif_proj.o

The script adb-push-run.sh pushes the executable, libraries, and test input to the Android
board (assuming there is only one device attached), runs the executable, and retrieves
the WAV file outputs back into ./out.

$ ./adb-push-run.sh
    + TOP_DIR=anki_victor_example
    + ADB=adb
    ++ adb devices
    ++ wc -l
    + '[' 3 -ne 3 ']'
    + adb shell rm -rf /data/anki_victor_example
    + adb shell mkdir /data/anki_victor_example
    + adb shell mkdir /data/anki_victor_example/build
    + adb shell mkdir /data/anki_victor_example/out
    + adb shell chmod 777 /data/anki_victor_example
    + adb shell chmod 777 /data/anki_victor_example/build
    + adb shell chmod 777 /data/anki_victor_example/out
    + adb push build/simple /data/anki_victor_example/build
    build/simple: 1 file pushed. 9.2 MB/s (2360784 bytes in 0.245s)
    + adb push build/libmmfx.a /data/anki_victor_example/build
    build/libmmfx.a: 1 file pushed. 9.4 MB/s (2306206 bytes in 0.234s)
    + adb push build/libmmif.a /data/anki_victor_example/build
    build/libmmif.a: 1 file pushed. 7.7 MB/s (121670 bytes in 0.015s)
    + adb push config.json /data/anki_victor_example/.
    config.json: 1 file pushed. 0.2 MB/s (493 bytes in 0.003s)
    + adb push sin1004hz_16khz.rmsdb_-24.wav /data/anki_victor_example/.
    sin1004hz_16khz.rmsdb_-24.wav: 1 file pushed. 8.8 MB/s (320044 bytes in 0.035s)
    + adb shell 'export LD_LIBRARY_PATH=anki_victor_example/build: && cd /data/anki_victor_example && ./build/simple'
    WARNING: linker: Warning: unable to normalize "anki_victor_example/build"
    WARNING: linker: Warning: unable to normalize ""
    WARNING: linker: /data/anki_victor_example/build/simple: unused DT entry: type 0xf arg 0x33c
    version=(anki_victor 001|mmfxver-localbuild)
    ReadConfigFile:  reading (config.json).......done
    sin: trying to open (./sin1004hz_16khz.rmsdb_-24.wav)...success
    rin: trying to open (./sin1004hz_16khz.rmsdb_-24.wav)...success
    ReadConfigFile:  reading (config.json).......done
    rout: trying to open (out/example_rout.wav)...success
    sout: trying to open (out/example_sout.wav)...success
    ...........
    --------------------
    diag name: (fdsearch_num_beams_to_search) = 13
    type: 4 (where 4 == SE_DIAG_UINT16)
    readWrite: 1 (where 1 == SE_DIAG_R)
    numElements: 1
    description: Number of beams we're searching in
    --------------------
    diag name: (fdsw_current_winner_beam) = 6
    description: Winning beam index, determined over time interval
    + rm -f out/example_rout.wav out/example_sout.wav
    + adb pull /data/anki_victor_example/out/example_rout.wav out/.
    /data/anki_victor_example/out/example_rout.wav: 1 file pulled. 12.1 MB/s (320364 bytes in 0.025s)
    + adb pull /data/anki_victor_example/out/example_sout.wav out/.
    /data/anki_victor_example/out/example_sout.wav: 1 file pulled. 3.1 MB/s (320364 bytes in 0.097s)







