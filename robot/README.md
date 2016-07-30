# Cozmo Firmware

The firmware is now normally build on the build servers. This document provides instructions on how to build locally for
development.

## System prerequisites

**General**
* Python3
 * ELFtools
 * Crypto
* GNU Make

**K02, NRF51 and NRF31**
* Keil 4 or greater

**Espressif**
* Espressif toolchain, see [Espressif readme](espressif/README.md)

## Building Firmware Locally

Start by pulling the server firmware assets down by running
```
make pull
```
This will place copies of the current build server created firmware for your branch in the `build` directory.

To run pre-build code generation steps run
```
make dev
```
To build K02 and NRF51 firmware, use Keil project

To build Espressif firmware run
```
make espBuild
```

Finally to generate the OTA file packaging all the firmware together, run
```
make ota
```

All binaries are put in the `build` directory
