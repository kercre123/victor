# victor

`victor` is the _code name_ for the next iteration of the Cozmo product line.

This repo contains code for the embedded firmware (syscon), robotics, animation and engine layers.
It currently contains a snapshot of the Cozmo 2.x app, but this is not supported or guaranteed to work.

If you're looking for the emdedded OS that runs on victor hardware, there is not much in the way of formal documentation yet, but you can start by looking at our repo of [cozmos-proprietary-patches](https://github.com/anki/cozmos-proprietary-patches).

If you need to work on Cozmo, you should look in [cozmo-one].

[cozmo-one]: https://github.com/anki/cozmo-one

## Getting Started

If you are new to Anki please also see the [New Hire Onboarding](https://ankiinc.atlassian.net/wiki/pages/viewpage.action?pageId=72614010) page.

Detailed documentation for building, running and debugging is available under `docs/`.

## Quick Start Guide

### Getting the code

You can fetch the code using `git`.  Some assets currently require `svn`, which will be installed automatically.

protip: You can organize directories however you want, but in practice it is useul to organize using a few subdirectories. If you're not sure what to do, we recommend organizing repos grouped by project under a top-level `src` dir in your home directory, for example:

```
/Users/chapados/
├── src
│   └── victor
│       ├── victor  <- **This is your victor repo**
│       ├── <other victor-related repos>
```

```
cd
mkdir -p src/victor
cd src/victor
git clone --recursive git@github.com:anki/victor.git
```

### Submodules
Fetch the submodules:

```
git submodule update --init --recursive
```

Alternatively when fetching the code:
```
git clone --recursive <git url>
```

### Building Victor

The underlying build system for victor is currently `CMake`.  The appropriate version of CMake and other dependencies required for building victor will be fetched automatically.

Configure your shell for command-line builds:

```
source project/victor/envsetup.sh
```

To build for embedded android (the default):

```
./project/victor/build-victor.sh

# you can also explicitly pass the platform
./project/victor/build-victor.sh -p android
```

To build for mac:

```
./project/victor/build-victor.sh -p mac

# To generate an Xcode project without building
./project/victor/build-victor.sh -p mac -g Xcode -C
```

We use a helper script to generate file lists to tell CMake what to build. Sometimes, when changing branches, CMake doesn't notice that it needs to regenerate build files. You can force it do to so by passing the `-f` flag:

```
./project/victor/build-victor.sh -f
```

See [build-instructions](docs/development/build-instructions.md) for a more thorough description of build options.

## Deploy Victor

1. First you need to enable adb connections to robot. This can be done over USB or wifi.

### USB
Connect cable to robot. Run `adb devices` to verify that only one device is available.

### Wifi
Connect your computer to the `AnkiTest2` access point (Password is "password"). If you don't know the IP address of your robot, connect to the robot with USB and run

```
adb shell ifconfig
```

The `inet addr` for `wlan0` is the robot's IP address.

You can then disconnect the cable and run

```
adb connect <Robot's IP address>
```

(If the operation times out, try connecting with USB, running `adb root`, and trying again.)

You should now see the robot's IP as a connected device when you run `adb devices`. (Disconnect USB if still connected.)

2. Deploy Engine, Robot, and assets to physical robot
```
./project/victor/scripts/deploy.sh
./project/victor/scripts/deploy-assets.sh
```

  If you want to deploy the release build,
```
ANKI_BUILD_TYPES=Release ./project/victor/scripts/deploy.sh
```

## Running Victor

1. Connect to the robot via adb over wifi

```
adb shell
```

2. Go to where the binaries are deployed.

```
cd /data/data/com.anki.cozmoengine
```

3. Start robot process

```
./robotctl.sh start
```

After starting the robot process, you should see/hear head and lift calibrating within a few seconds. If you don't, restart the robot process by running:
```
 ./robotctl.sh restart
 ```

4. Start anim process
```
./animctl.sh start
```

5. Start engine process

```
./cozmoctl.sh start
```

6. You can view output from all processes by running `logcat`


## Connect to Victor with Webots 

1. Modify the world `cozmo2Viz.wbt` so that `forcedRobotIP` and `engineIP` are both the same as the robot's IP address. 

2. Run the world to connect to the robot.

3. Ad-hoc connections to a robot are not yet supported. If you disconnect Webots and you want to connect again, you will first need to restart the engine, animation and robot processes with

```
./cozmoctl.sh restart
./animctl.sh restart
./robotctl.sh restart
```


## Updating syscon

Most developers shouldn't have to do this. Better to ask someone in hardware or Al or Kevin Y.

1. To build syscon, run `vmake.sh` in `robot2/`. 

2. If the robot has previously run the robot process since last boot then you'll need to first reboot the robot. 

3. Run `dfu.sh` in `robot2/`. There should be a bunch of messages indicating transfer of data. Might need to run twice.
