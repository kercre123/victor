# victor

`victor` is the _code name_ for the next iteration of the Cozmo product line. This repo contains code for the embedded firmware (syscon), robotics, animation and engine layers.

If you're looking for the embedded OS that runs on victor hardware, there is not much in the way of formal documentation yet, but you can start by looking at our repo for [vicos-oelinux](https://github.com/anki/vicos-oelinux).

If you need to work on Cozmo, you should look in [cozmo-one](https://github.com/anki/cozmo-one).

For more information about the system architecture, components, organization at a high level, check out the [Victor System Architecture](docs/architecture/README.md) page.

## Getting Started

If you are new to Anki, please also see the [New Hire Onboarding](https://ankiinc.atlassian.net/wiki/pages/viewpage.action?pageId=72614010) page.

More documentation related to development, building, and running is available under [docs](/docs). See also the [Frequently Asked Questions](/docs/FAQ.md) page.

### Getting the code

You can fetch the code using `git`.  Some assets currently require `svn`, which will be installed automatically.

Pro tip: You can organize directories however you want, but in practice it is useful to organize using a few subdirectories. If you're not sure what to do, we recommend organizing repos grouped by project under a top-level `src` dir in your home directory, for example:

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

### ADB setup

1. Make sure you do not have Android SDK or Android NDK installed from `brew`:

    ```
    brew cask uninstall android-sdk
    brew cask uninstall android-ndk
    ```

    This way you only have one version of `adb` on your system. (If you have two they fight with each other, i.e. disconnect)

1. Get the Anki-blessed Android SDK and Android NDK by running the following commands:

    ```
    ./tools/build/tools/ankibuild/android.py --install-sdk
    ./tools/build/tools/ankibuild/android.py --install-ndk
    ```

1. Make sure the following is in your `~/.bash_profile`:

    ```
    ANKI_ANDROID_ROOT=~/.anki/android
    ANDROID_SDK_REPOSITORY=${ANKI_ANDROID_ROOT}/sdk-repository
    export ANDROID_HOME=${ANDROID_SDK_REPOSITORY}/`/bin/ls $ANDROID_SDK_REPOSITORY/ | tail -1`
    export ANDROID_ROOT=$ANDROID_HOME

    export ANDROID_NDK_REPOSITORY=${ANKI_ANDROID_ROOT}/ndk-repository
    export ANDROID_NDK_ROOT=${ANDROID_NDK_REPOSITORY}/`/bin/ls $ANDROID_NDK_REPOSITORY/ | tail -1`
    export ANDROID_NDK_HOME=$ANDROID_NDK_ROOT
    export ANDROID_NDK=$ANDROID_NDK_ROOT
    export NDK_ROOT=$ANDROID_NDK_ROOT
    export PATH=${PATH}:${ANDROID_HOME}/platform-tools  # for adb
    export PATH=${PATH}:${ANDROID_HOME}/anki/bin # For tools like buck
    ```

1. Run `source ~/.bash_profile` or open a new terminal. `adb` should now be in your path.

### Miscellaneous setup

1. To speed up builds, [install `ccache`](/docs/ccache.md) on your system and the build system will automatically start using it.

1. Add the following to your `~/.bash_profile`:

    ```
    source <PATH_TO_YOUR_VICTOR_REPO>/project/victor/scripts/usefulALiases.sh
    ```

    This will give you access to very useful aliases for the various build/deploy scripts. These aliases will be referenced extensively in this doc.

## Building Victor

If you're a developer, it is highly recommended that you check out the [Victor Build System Walkthrough](/docs/build-system-walkthrough.md) doc to familiarize yourself with the build system. The underlying build system for victor is [`CMake`](https://cmake.org/).  The appropriate version of CMake and other dependencies required for building victor will be fetched automatically.

1. To build for embedded android (the default), ensure you are in the `victor` directory and run

    ```
    victor_build
    ```

    If this command fails for some reason, try running `project/victor/build-victor.sh -f -X`<sup>1</sup>.

1. To build for mac, run

    ```
    project/victor/build-victor.sh -p mac
    ```

    If this command fails for some reason, try running `project/victor/build-victor.sh -p mac -f -X`<sup>1</sup>.

1. If you want to force a 'clean' build, the brute-force method is to run

    ```
    rm -rf _build EXTERNALS generated
    ```
    
    then rebuild. Note that it will now take 10-20 minutes to rebuild.
    
1. To generate an Xcode project without building, run

    ```
    project/victor/build-victor.sh -p mac -g Xcode -C
    ```

<sup>1.</sup> Note about the `-f` and `-X` flags: We use a helper script to generate file lists to tell CMake what to build. Sometimes, when changing branches, CMake doesn't notice that it needs to regenerate build files. The `-f` flag forces it to do so. Also, asset files can be force-updated by using the `-X` flag.

For a more thorough description of build options, run `project/victor/build-victor.sh -h` or check out the [build-instructions](/docs/development/build-instructions.md) document.

## Deploying Victor

### Connecting over Wifi

1. Make sure your laptop is connected to the `Anki5Ghz` wifi network. If you are on a home network, follow the instructions [here](/tools/victor-ble-cli#configuring-wifi).

1. Power on your robot and wait until the eyes appear on the screen. Then click the backpack button once to show the debug display. The robot's IP address appears on the first line.

    If that method doesn't work, you can also try running

    ```
    tools/victor-ble-cli/vic_show_ip.sh VICTOR_xxxxxx
    ```

    where `xxxxxx` is the bluetooth ID of the robot (which is displayed on the screen when the robot is powered up). You may need to put the robot physically near your computer when you run the script.

    The `inet addr` for `wlan0` is the robot's IP address.

1. Connect to the robot over `adb` by running

    ```
    adb connect <Robot's IP address>
    ```

    You should now see the robot's IP as a connected device when you run `adb devices`.

1. If the operation times out or otherwise fails, power cycle the robot and try again. Alternatively, you can connect to the robot via the BLE tool and restart `adb` on the robot. To do this, navigate to the `tools/victor-ble-cli/` directory and run `node index.js`. Then run `scan`, and when you see your robot on the list, run `connect VICTOR_xxxxxx`. Then run `restart-adb` and try the connection process again.

### Deploying binaries and assets to physical robot

1. Run `victor_stop` to stop the processes running on the robot.

1. Run `victor_deploy` to deploy the binaries and asset files to the robot.

1. If the operation times out, power cycle the robot and try again.

1. If you want to deploy the debug build (you should not normally have to do this), run

    ```
    ANKI_BUILD_TYPE=Debug ./project/victor/scripts/deploy.sh
    ```

## Running Victor

Once the binaries and assets are deployed, you can run everything by running `victor_start`.

You can view output from all processes by running `victor_log`.

## Connect to Victor with Webots

Note that we have floating licenses for Webots. Check with your friendly producers to see if it's OK for you to install and use Webots, since you may be taking a license from someone who really needs it. 

1. First, make sure you have built for mac (see instructions above) on the same branch as the binaries on the physical robot.

1. Open the world [cozmo2Viz.wbt](/simulator/worlds/cozmo2Viz.wbt) in a text editor, and set the field `engineIP` to match your robot's IP address. 

1. Run the world to connect to the robot. The processes must be fully up and running before connecting with webots. If in doubt, stop the world and run `victor_restart` first.

## Running Victor in Webots (simulated robot)

See [simulator/README.md](/simulator/README.md).

## Updating syscon (body firmware)

Most developers shouldn't have to do this. Better to ask someone in hardware or Al or Kevin Y.

1. To build syscon, run `vmake.sh` in [robot/](/robot). 

1. If the robot has previously run the robot process since last boot then you'll need to first reboot the robot. 

1. Run [dfu.sh](/robot/dfu.sh) in [robot/](/robot). There should be a bunch of messages indicating transfer of data. Might need to run twice.

## Having trouble?

Check out the [Frequently Asked Questions](/docs/FAQ.md) page. When you get new questions answered, consider adding them to the list to help others!
