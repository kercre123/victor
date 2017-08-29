# TODO: Merge with main README.md after fork

# Victor Setup


## ADB setup

Since the Android SDK and NDK are automatically downloaded with `./configure.py` you should be using `adb` from there. Make sure you do not have android-sdk installed from `brew` by running. 

`brew uninstall android-sdk`

That way you only have one version of adb on your system. (If you have two they fight with each other, i.e. disconnect)


Make sure the following is in your ~/.bash_profile
```
ANKI_ANDROID_ROOT=~/.anki/android
ANDROID_SDK_REPOSITORY=${ANKI_ANDROID_ROOT}/sdk-repository
export ANDROID_HOME=${ANDROID_SDK_REPOSITORY}/`ls $ANDROID_SDK_REPOSITORY/| tail -1`
export ANDROID_ROOT=$ANDROID_HOME

export ANDROID_NDK_REPOSITORY=${ANKI_ANDROID_ROOT}/ndk-repository
export ANDROID_NDK_ROOT=${ANDROID_NDK_REPOSITORY}/`ls $ANDROID_NDK_REPOSITORY/| tail -1`
export ANDROID_NDK_HOME=$ANDROID_NDK_ROOT
export ANDROID_NDK=$ANDROID_NDK_ROOT
export NDK_ROOT=$ANDROID_NDK_ROOT
export PATH=${PATH}:${ANDROID_HOME}/platform-tools  # for adb
export PATH=${PATH}:${ANDROID_HOME}/anki/bin # For tools like buck
```

Do `source ~/.bash_profile` or open a new terminal and `adb` should now be in your path


## Build Victor

1. Build engine and robot processes (in Debug)
```
./project/victor/build-victor.sh
```

  If it doesn't work (which may happen because new files were added, either by you or after sync) try running with `-f` to force.

2. To build in Release 
```
./project/victor/build-victor.sh -c Release
```


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

3. Run robot process

```
./robotctl.sh start
```

4. Run engine process

```
./cozmoctl.sh start
```

5. You can view output from all processes by running `logcat`


## Connect to Victor with Webots 

1. Modify the world `cozmo2Viz.wbt` so that `forcedRobotIP` and `engineIP` are both the same as the robot's IP address. 

2. Run the world to connect to the robot. (For the time, being webotsControllers still need to be built using `./configure.py build -p mac -2`)

3. Ad-hoc connections to a robot are not yet supported. If you disconnect Webots and you want to connect again, you will first need to restart the engine and robot processes with

```
./cozmoctl.sh restart
./robotctl.sh restart
```


## Updating syscon

Most developers shouldn't have to do this. Better to ask someone in hardware or Al or Kevin Y.

1. To build syscon, run `vmake.sh` in `robot2/`. 

2. If the robot has previously run the robot process since last boot then you'll need to first reboot the robot. 

3. Run `dfu.sh` in `robot2/`. There should be a bunch of messages indicating transfer of data. Might need to run twice.


