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

## Guidelines for writing Espressif Firmware
The Espressif SoC is a slightly unusual beast with a lot of power but it is not idiot proof. A number of precautions
need to be taken.

### For application code

#### Code structure
* All application code must be in either a task or a timer
* All tasks and timers *should* complete in less than 2ms and **must** complete in less than 500ms.
* Reoccuring timers should not be scheduled more often than every 5ms.
* There are 3 user task levels: 0, 1, 2.
 * All level 2 tasks queued run before any queued level 1 tasks and so on.
 * Task level 2 is reserved for HAL functions
* Timers which are due are higher priority than tasks.
* Tasks and timers *do not* pre-empt each other. A new task or timer cannot start until the runing task / timer has ended.
* Interrupts must not be locked out for more than 10us at a time or WiFi will crash

#### Performance and Memory
* All application code should have the `ICACHE_FLASH_ATTR` decorator.
* The Espressif is not especially fast and fetching code is relatively slow, however, it has a relatively large amount
of RAM so when making computation/code-size/RAM use trade offs, generally take the path that uses more RAM.
* Heep (malloc and free) is available but should be used sparingly and cautiously.
* Code size:
 * Since the Espressif uses external SPI flash, there is effectively unlimited code memory.
 * However, loading code from SPI flash is relatively slow and there is a limited code cache in local chip RAM. Hence inner loops and code which executes often should be kept relatively small to reduce cache thrashing.

### For HAL
In addition to all the guidelines above:
* Interrupts must not be disabled for more than 10us.
* No ISR can run for more than 10us.
* Register reading and writing is surprisingly slow, cache where possible, etc.
* All HAL code which runs often should *not* have the `ICACHE_FLASH_ATTR` decorator.
