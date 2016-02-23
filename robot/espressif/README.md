# Espressif firmware
Firmware for the Espressif Wifi radio SoC

The instructions are primarily for building on Linux, as of this writing, Ubuntu 15.

## How to build a build environment.

### System dependancies
Install needed packages which may include the following among others. There will likely be some iteration with
installing system dependancies while trying to build the toolchain.

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
The Anki scripts are written primarily targeting python3 but the Espressif scripts target python2 so you'll need to
have both installed and have ```python2``` and ```python3``` in your path. On OSX you need to create a python2 symlink
```
cd /usr/bin
sudo ln -s python python2
```

#### Serial tools
If you will be programming the Espressif directly over serial you'll need to install the open source esptool
```
sudo pip install git+https://github.com/themadinventor/esptool.git```

For some scripts you may also need to install Python3 pyserial by running
```
sudo pip3 install pyserial
```

### GCC Tool Chain
We have build custom copies of the GCC toolchain for the Xtensa LX106 CPU for both OSX and Linux.
1. [Download from Dropbox](https://www.dropbox.com/sh/7zmuion2gti5bed/AABrkJ5n9XsZ91mWlCU9w09qa?dl=0)
2. Make a destination directory for the toolchain
```
sudo mkdir -p /opt
sudo chown `whoami`: /opt
```
3. Extract the toolchain there
```
cd /opt
tar xjf <PATH TO DOWNLOADED TOOLCHAIN>
```

#### Building the toolchain from scratch
You don't want to do this, but if you have to, look at the _README.md_ in the Dropbox folder for the OS you need to build the tool chain for.

## Build the firmware

```
cd <PRODUCTS COZMO REPOSITORY>/robot
make esp
```
The Makefile sets the appropriate variables for our project.

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
* All application code must be in a task. There are two Anki specific task API wrappers available for this:
 * BackgroundTask which is a round robin task system for normal application operation
 * Foreground task which is for one off tasks that need to happen at a higher priority occasionally
* Timers may not be used unless all the timer callback does is post a foreground task and reocuring timers should not have intervals less than 500ms. Timers which are due are higher priority than tasks.
* All tasks *should* complete in less than 2ms and **must** complete in less than 5ms.
* Tasks and timers *do not* pre-empt each other. A new task or timer cannot start until the running task / timer has
  ended.
* Interrupts must not be locked out for more than 10µs at a time or WiFi will crash

#### Performance and Memory
* The Expressif has a fast clock speed but low cycle efficiency and most significantly fetching code is relatively slow as it comes from an external QSPI flash chip when not in cache, however, it has a relatively large amount of RAM so when making computation/code-size/RAM use trade offs, generally take the path that uses more RAM.
* Heep (malloc and free) is available but should be used sparingly and cautiously.
* Code size:
 * Since the Espressif uses external QSPI flash, there is effectively unlimited code memory.
 * However, loading code from SPI flash is relatively slow and there is a limited code cache in local chip RAM. Hence
  inner loops and code which executes often should be kept relatively small to reduce cache thrashing.

### For HAL
In addition to all the guidelines above:
* Interrupts must not be disabled for more than 10us.
* No ISR can run for more than 10µs.
* Register reading and writing is surprisingly slow, cache where possible, etc.
* All HAL code which runs often should *not* have the `ICACHE_FLASH_ATTR` decorator.
