# Espressif firmware
Firmware for the Espressif Wifi radio SoC

The instructions are primarily for building on Linux, as of this writing, Ubuntu 15.

##How to build a build environment.

###System dependancies
Install needed packages which may include the following among others. There will likely be some iteration with
installint system dependancies while trying to build the toolchain.

#### 32-bit Linux
```
git autoconf build-essential gperf bison flex texinfo libtool libtool-bin libncurses5-dev wget gawk libc6
```
#### 64-bit Linux
```
git autoconf build-essential gperf bison flex texinfo libtool libtool-bin libncurses5-dev wget gawk libc6-dev-amd64 libexpat-dev
```
#### OSX Homebrew
```
gperf bison flex texinfo libtool wget gawk crosstool-ng autoconf automake gcc
```

### Python
The Anki scripts are written primarily targetting python3 but the Espressif scripts target python2 so you'll need to
have both installed and have ```python2``` and ```python3``` in your path. On OSX you need to create a python2 symlink
```
cd /usr/bin
sudo ln -s python python2
```

Python serial will also be required. If pip is not already installed running
```
sudo easy_install pip
```
Install pyserial by running
```
sudo pip install pyserial  # for python2 for programming the Espressif
sudo pip3 install pyserial # Optionally for python3 for running test scripts etc.
```

### Checkout and build crosstool-NG toolchain
Setup a folder for it to live in
```
sudo mkdir -p /opt/Espressif
sudo chown `whoami`: /opt/Espressif
```
Checkout the Espressif C++ crosstool-NG repository / branch
```
cd /opt/Espressif
git clone -b lx106 git://github.com/jcmvbkbc/crosstool-NG.git
cd crosstool-NG
git checkout lx106-g++ # Checkout the branch which includes C++ compiler in addition to C99 compiler
./bootstrap
./configure --prefix=`pwd`
make
make install
./ct-ng xtensa-lx106-elf
./ct-ng build
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
