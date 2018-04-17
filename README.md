# victor

**If you have a DVT3 robot, some of these instructions may be outdated. Refer to confluence for the latest. (Their contents should get merged into the repo when things stabilize.)**  
[https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/221151299/Victor+Build+Setup](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/221151299/Victor+Build+Setup)
[https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/368476326/Victor+DVT3+ssh+connection](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/368476326/Victor+DVT3+ssh+connection)



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

**DEPRECATED: We now use ssh instead of adb to connect to robots.**

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

### Install shared victor ssh key

If your robot is >= 0.9.1 then it has a built-in ssh key. To interact with the robot you need to know its IP address and you must have the ssk key on your computer.

1. Download the private key to `~/.ssh/id_rsa_victor_shared`
```
curl -sL -o ~/.ssh/id_rsa_victor_shared https://www.dropbox.com/s/mgxgdouo0id6j9m/id_rsa_victor_shared?dl=0
chmod 600 ~/.ssh/id_rsa_victor_shared
ssh-add -K ~/.ssh/id_rsa_victor_shared
``` 

If you're on OS X Sierra you may want to add the `ssh-add` line to your `.bashrc` or `.bash_profile` since adding to keychain with `-K` is broken in Sierra.

1. Build, deploy, and run commands as normal (See []()). You can specify the target robot with `-s` on most of the deploy and run commands or you can set the `ANKI_ROBOT_HOST` environment variable in your shell to set a default host.

```
export ANKI_ROBOT_HOST="<ip address>"
```

1. You can open an ssh session on the robot as follows:

```
ssh root@${ANKI_ROBOT_HOST}
```

or execute remote commands. e.g. `ssh root@${ANKI_ROBOT_HOST} "logcat -vthreadtime"`


### Connecting over Wifi

**If you have a DVT3 robot you should no longer be using `victor-ble-cli` for configuring wifi over bluetooth. Robots should already connect automatically to the `AnkiRobits` network, but if you need to attach to a diifferent network use the `mac-client` referred to in**  
[https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/323321886/Victor+OTA+update+using+Mac-Client+tool#VictorOSandOTAupdateusingMac-Clienttool-OTA](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/323321886/Victor+OTA+update+using+Mac-Client+tool#VictorOSandOTAupdateusingMac-Clienttool-OTA))


1. Make sure your laptop is connected to the `Anki5Ghz` wifi network. If you are on a home network, follow the instructions [here](/tools/victor-ble-cli#configuring-wifi).

1. Power on your robot and wait until the eyes appear on the screen. Then click the backpack button once to show the debug display. The robot's IP address appears on the first line.

    If that method doesn't work, you can also try running

    ```
    tools/victor-ble-cli/vic_show_ip.sh VICTOR_xxxxxx
    ```

    where `xxxxxx` is the bluetooth ID of the robot (which is displayed on the screen when the robot is powered up). You may need to put the robot physically near your computer when you run the script.

    The `inet addr` for `wlan0` is the robot's IP address.

1. We no longer use adb to connect to the robot. Instead we use SSH which is more reliable. You need to do this ONE-TIME operation for each different robot you want to connect to, on the laptop you're connecting from. Run this script:

    ```
    ./tools/victor-ble-cli/vic_set_robot_key.sh VICTOR_xxxxxx
    ```

    where 'xxxxxx' is the bluetooth ID of the robot (as mentioned above, for example VICTOR_1a3d273d, or "VECTOR R2D2"). This script will generate an SSH key pair, install it on your laptop, and then connect to the robot and install the public key on the robot. It will also modify your `~/.ssh/config` file to add some lines that will prevent unnecessary prompts and warnings. Finally, it will show you the robot's IP address, and write that IP address to a file on your laptop.

1. Once that one-time step is done, all of the existing scripts (victor_start, victor_deploy, victor_log, etc.) will work as before, but without using adb.<br/><br/>You can also run a command on the robot with:

    ```
    ssh root@<robotIP> <command> <arguments>
    ```

    where 'robotIP' is the robot's IP address. You can also shell in to the robot with:

    ```
    ssh root@<robotIP>
    ```

1. Unless you're running the `ssh` command, however, you won't need to enter the robot IP address again, for the other scripts, e.g. victor_log, victor_deploy, etc. If you want to switch to another robot, first make sure the vic_robot_set_key script (above) has been run on your laptop for that robot, if you haven't done so already. Then, run

    ```
    ./project/victor/scripts/victor_set_robot_ip.sh <robotIP>
    ```

    to switch to the new robot's IP address. From that point forward the victor_log, etc. scripts will use that new IP address.

1. For convenience when working with multiple robots, or if you just want to be sure you're targeting a specific robot, almost all of the commands accept a robot IP with the -s option. For example:

    ```
    victor_deploy_release -s 192.168.43.45
    ```

    will deploy the release version to the robot at 192.168.43.45.

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
