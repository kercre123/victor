# Espressif firmware
Firmware for the Espressif Wifi radio SoC

#How to build a build environment.

##STEP 0: Environment
Install needed packages which may include the following among others

```
build-essential gperf bison flex texinfo libtool libncurses5-dev wget gawk libc6-dev-amd64 python-serial libexpat-dev libisl-dev libcloog-isl-dev libmpc-dev
```

Some iteration between building the xtensa toolchain and installing system packages will almost doubtless be needed to
make this work.

## STEP 1: Checkout coretech-external
Most of the packages have already been collected in the [coretech-external repository](https://github.com/anki/coretech-external).
The Makefile assumes that this repository is in the same folder as the product-cozmo folder.

##STEP 2: Build the xtensa-toolchain
Get and build a copy of the xtensa build toolchain here:
https://github.com/jcmvbkbc/xtensa-toolchain-build
... you're building the lx106 version.  You will also need the patched GCC here: https://github.com/jcmvbkbc/gcc-xtensa/commits/call0-4.8.2
The commands you will need are as follows:
```
cd ~/Documents/GitHub/coretech-external/espressif/xtensa-toolchain-build
git clone --depth=1 https://github.com/jcmvbkbc/gcc-xtensa.git gcc-xtensa gcc-4.9.1
./prepare.sh lx106
./build-elf.sh lx106

cd ~/Documents/GitHub/coretech-external/espressif/other/esptool
make
```
This is where you will probably have to iterate with installing system dependancies.
You may also need to edit the makefile in other/esptool.

## STEP 3: Build the firmware

```
cd ~/Documents/GitHub/products-cozmo/robot/espressif
make build
```

Firmware may be installed onto a module by running:
```
make burn
```
After setting the com port correctly in the makefile.

********************************************************************************



##Appendix A: Building on OSX. (Copied verbatim from [cnlohr's instructions](https://github.com/cnlohr/ws2812esp8266/blob/master/README.md))
Ingo Randolf found that this also works on OSX with the following commands:


instead of wget one can use curl for downloading files:
curl -O http://file-to-download.zip

##1. building gcc

In order for gcc to build I had to specify where to find mpc, mpfr, gmp. all of those libraries are installed via macports, so they are at /opt/local/include and /opt/local/lib.

In build-gcc.sh I appended this to the line which call the configure command for gcc:
```--with-mpc=/opt/local --with-mpfr=/opt/local --with-gmp=/opt/local```

The full line looks like this:
```../../$GCC/configure --target=$TARGET --prefix=$PREFIX \
-enable__cxa_atexit --disable-shared --disable-libssp --enable-languages=c "$@"  --with-mpc=/opt/local --with-mpfr=/opt/local --with-gmp=/opt/local```


##2. ccache-install.sh

In ccache-install.sh i needed to change the last line to:

```find ./ -type f | xargs -I{} ln -sf ../../../ccache.sh ../ccache/{}```

  "find" needs a path as the seconds argument, xargs option -i is deprecated, -I must be used.
