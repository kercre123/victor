# Espressif firmware
Firmware for the Espressif Wifi radio SoC

The instructions are primarily for building on Linux, as of this writing, Ubuntu 15.

##How to build a build environment.

###STEP 0: Environment
Install needed packages which may include the following among others

```
build-essential gperf bison flex texinfo libtool libncurses5-dev wget gawk libc6-dev-amd64 python-serial libexpat-dev libisl-dev libcloog-isl-dev libmpc-dev
```

Some iteration between building the xtensa toolchain and installing system packages will almost doubtless be needed to
make this work.

### STEP 1: Checkout coretech-external
Most of the packages have already been collected in the [coretech-external repository](https://github.com/anki/coretech-external).

After setting up coretech-external, you need to add an environmental variable ```CORETECH_EXTERNAL_DIR``` to tell
scripts where to find it.

###STEP 2: Build the xtensa-toolchain
Get and build a copy of the xtensa build toolchain here:
https://github.com/jcmvbkbc/xtensa-toolchain-build
... you're building the lx106 version.  You will also need the patched GCC here: https://github.com/jcmvbkbc/gcc-xtensa/commits/call0-4.8.2
The commands you will need are as follows:
```
cd $CORETECH_EXTERNAL_DIR/espressif/xtensa-toolchain-build
git clone --depth=1 https://github.com/jcmvbkbc/gcc-xtensa.git gcc-4.9.1
./prepare.sh lx106
./build-elf.sh lx106
```
This is where you will probably have to iterate with installing system dependancies.

Now install the xtensa toolchain:
```
sudo cp -r build-lx106/root/* /usr/local/
```

## Build the firmware

```
cd <PRODUCTS COZMO REPOSITORY>/robot/espressif
./build.sh
```
The build.sh script sets the appropriate variables for our project before calling the Espressif Makefile.

Firmware may be installed onto a module by running:
```
./burn.sh <COM PORT>
```
Where _COM PORT_ is the serial port the ESP8266 programming port is connected to.

********************************************************************************



##Appendix A: Building on OSX.
See [cnlohr's instructions](https://github.com/cnlohr/ws2812esp8266/blob/master/README.md)
