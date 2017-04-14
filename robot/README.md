# Cozmo Firmware

Cozmo 1.x firmware for inclusion in the app is now build on the build servers. This document describes how the multiple
firmware binaries are each built and then packed together both locally and on the server.

## Overview
Cozmo has 3 processors on board:
* Body aka NRF51 aka syscon
* RTIP (real time image processor) aka K02
* WiFi aka Espressif

Additionally each cube has it's on NRF31 processor.

Each processor needs firmware and some of the firmware images rely on sub-images being built first. Because the
firmware for all processors except the Espressif is built via Keil on Windows, much of the firmware build is done
in Windows. However, [CLAD](../tools/message-buffers/README.md) generated code is used extensively in the
firmware and it must be run in a unix like environment. Debugging strings in the firmware are also replaced with 
a shared string table by [AnkiLogPP](tools/ankiLogPP.py) tool. Finally, the firmware images are packed together
into a secure _.safe_ file.

### Build Flavors
The firmware build supports multiple build "flavors", at time of this writing the supported flavors are

* DEVELOPMENT – default
* RELEASE – For inclusion in shipping app builds
* SIMULATOR – for simulated robot only, not real firmware
* FACTORY – Firmware to be installed on robot in factory for factory tests and out of box

The flavor is defined at the start of the build process and should apply to all the firmware images build until 
the flavor is reconfigured.

["Feature Flags"](https://ankiinc.atlassian.net/wiki/display/COZMO/Cozmo+Feature+Flags) are turned on or off by the
build flavor and will cause some code to be compiled in or out. See below for more details.

-------------------------------------------------------------------------------

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

### Download server firmware assets
If you aren't going to modify all the firmware images, you can avoid building them all locally by starting with
assets from the build server.

Pull the server firmware assets down by running
```
make pull
```
Or if GNU make is not available (such as on Windows)
```
python3 pull_server_firmware.py
```
This will place copies of the current build server created firmware for your branch in the `build` directory.

### Pre-build

Before building any firmware, CLAD code generation must be run and the debug string table –
[AnkiLogPP](tools/ankiLogPP.py) – must be updated, and the build flavor must be set. The prebuild step does all these as

```
make dev [BUILD_TYPE=<type>]
```

If no build type is specified, the default is DEVELOPMENT. The flavor will remain in affect until you run `make dev`
again.

### Keil build

To build K02 and NRF51 firmware, use Keil workspace `cozmo.uvmpw`

### Espressif build
To build Espressif firmware run
```
make espBuild
```

### OTA File

Finally to generate the OTA file packaging all the firmware together, run
```
make ota
```

## Server builds
The server build process goes roughly as follows
 1. Run `make server.1` on an OSX server and tar up the `generated` folder.
 2. On a windows server: untar the generated folder, build Keil workspace and tar up built binaries etc.
 3. On an OSX server, untar the binaries from Keil and run `server.3` and tar up the output.

## Accessory (Cube) firmware
* Ask [Nathan](mailto:nathan@anki.com?subject=Building%20Accessory%20firmware) until someone writes documentation.

## Fixture firmware
* Ask [Craig](mailto:crohe@anki.com?subject=Building%20fixture%20firmware) until someone writes documentation.

-------------------------------------------------------------------------------

## Feature flags
The firmware now supports compile time feature flags similar to the ones in engine. See the
[engine documentation](https://ankiinc.atlassian.net/wiki/display/COZMO/Cozmo+Feature+Flags)
for an introduction. The flags are defined in the file
```
robot/include/anki/cozmo/robot/buildTypes.h
```
A feature flag should have a descriptive name, e.g. `ENABLE_DIAGNOSTIC_FACE_MENUS` which is #defined as either 1 or 0
in each build flavor.

-------------------------------------------------------------------------------

## Building Factory Fimrware

Requirements:
 1. Same tools as regular firmware
 2. Be on a Factory branch.

### Build

The following process is known to work. Some steps may be redundant, but this is safe.

### Clad generation and build flags.

```
make clean
make dev BUILD_TYPE=FACTORY
```

This must be done before the other steps to ensure the FACTORY flag is set.

### Keil build

Use Keil on windows.
`Project->Open Project` to load workspace "cozmo.uvmpw"
`Project->Batch Build -> Rebuild` to build all images

### Espressif build

```
make esp_factory BUILD_TYPE=FACTORY
```
This generates `build/esp.factory.bin`

 

### Optional (OTA File)

To generate a `factory_upgrade.safe` file which can be used to OTA a new factory image:

```
make factory_upgrade BUILD_TYPE=FACTORY
```

Don't do this unless you really need to. Factory images should normally only be installed at the factory.


-------------------------------------------------------------------------------

## Trace Strings

To save code space and bandwidth, debugging strings in the firmware can be converted into a shared string table by the
[AnkiLogPP](tools/ankiLogPP.py) tool. 
1. Walks over all the firmware looking for macros as defined in `include/anki/cozmo/robot/logging.h`
2. If running in preprocessor mode (only during `make dev` prebuild step)
3. Generates the string table json for the app / python tools to use to format trace messages.

If a conflict in the string table is detected in preprocessor mode, it will be resolved. If not in preprocessor mode
(such as on the build server) it will cause an error so some merge conditions may require you to re-run `make dev`
locally and commit the preprocessor changes to resolve them.
