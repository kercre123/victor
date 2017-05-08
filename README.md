# cozmo-one

Cozmo Robot, Engine, and Unity UI/Game code. 

If you are new to Anki please also see the [New Hire Onboarding](https://ankiinc.atlassian.net/wiki/pages/viewpage.action?pageId=72614010) page

If you are contributing to the Unity/C# codebase, please read [Unity C# Coding Guidelines](https://github.com/anki/cozmo-one/wiki/Eng---Unity-C%23-Coding-Guidelines)
and follow [Unity Best Practices](https://github.com/anki/cozmo-one/wiki/Eng-Unity-Best-Practices).

# Building cozmo-one

### Brew

First install [brew](http://brew.sh/). Then use brew to install the following dependencies:

    brew install cmake
    brew install python3

### Android NDK and SDK

The Android NDK and SDK are required to build for Android.  They will be automatically installed for you when you run `./configure.py` for the Android platform.  You can also install them with:
```
./tools/build/tools/ankibuild/android.py --install-sdk
./tools/build/tools/ankibuild/android.py --install-ndk
```

You don't need to, but you may want to add the Android SDK platform tools to your path in `~/.bash_profile` with:
```
export ANDROID_HOME=~/.anki/android/sdk-repository/android-sdk-macosx-anki-r2/platform-tools
export PATH=$PATH:$ANDROID_HOME/platform-tools
```

### Unity

We are using [Unity 5.3.5p8](https://unity3d.com/unity/qa/patch-releases/5.3.5p8). Make sure you also install the iOS and Android components.

Right now we use one scene and load assets by code. The main scene is located here

    /unity/Cozmo/Assets/Scenes/Bootstrap.unity

### Xcode

We are using XCode Version 8.1. Install from the OS X App Store. Make sure you open XCode at least once after installing / updating because it may ask for accepting terms of service before permitting us to run it from build scripts.

### Build Script

To run the configure python scripts you'll need xcpretty.

    sudo gem install xcpretty

Build everything from the cozmo-game folder.

    cd <path-to-cozmo-repository>
    ./configure.py build

### [Webots](https://www.cyberbotics.com/overview)

Webots is what we use for simulation and visualization. It is also used to run the engine on desktop/Mac when we connect to a physical robot.

We are currently using [version 8.5.3](https://www.cyberbotics.com/archive/mac/webots-8.5.3.dmg). You will need to get a license from IT.

Webots runs concurrent processes for each active simulation object via "controllers". Cozmo simulations consist of the following controllers which can be found in /simulator/controllers. In order to debug with breakpoints you'll need to first attach to the appropriate process.

 * webotsCtrlGameEngine - Engine
 
 * webotsCtrlRobot - Robot
 
 * webotsCtrlLightCube - LightCube

 * webotsCtrlViz - Visualization overlays and display windows
 
 * webotsCtrlKeyboard - Keyboard interface for communicating with engine in place of the Unity app. Useful for quick headless (i.e. no Unity UI/Game) development.
 
 * webotsCtrlDevLog - For visualizing logs recorded from an application run. (Only used in devLogViz.wbt)
 
 * webotsCtrlBuildServerTest - For Webots-based tests (that are executed on the build server with every PR)

The Webots worlds can be found in /simulator/worlds

Useful worlds:

 * cozmoViz.wbt - Includes a keyboard controller and the engine. Used to talk to a physical robot.

 * cozmoVizForUnity.wbt - runs the engine. Use this if you want to run Unity to talk to a physical robot.

 * PatternPlay.wbt - Simulated environment for a virtual robot. Useful for using Unity to run simulated games. You can create your own versions of this ideal for the game you are testing.

 * remoteAnimationWorld.wbt - Used by the animation tool to see your animation on a simulated robot.
 
 * devLogViz.wbt - For visualizing logs recorded from an application run. Useful for debugging issues reported by QA.

# Optional Info

### configure.py Commands

The basic commands are as follows:

    ./configure.py generate    # (default) create the Xcode projects and workspaces.
    ./configure.py build       # generate, then use xcodebuild to build the generated projects (including Unity on iOS).
    ./configure.py install     # generate, build, then install the app on a connected ios device.
    ./configure.py run -p ios  # generate, build, install, then debug the app on a connected ios device using lldb.
    ./configure.py uninstall   # uninstall the app from a connected device.
    ./configure.py clean       # use xcodebuild to clean the generated projects (assuming they exist).
    ./configure.py delete      # delete all generated projects and compiled files.
    ./configure.py wipeall!    # delete, then wipe all ignored files in the entire repository

Only generate and delete are really useful for C++ developers since the other commands are just duplicates of what you have in Xcode. C# developers might find the terminal commands easier to work with, and eventually the terminal versions should be cross-platform.

KNOWN ISSUE : if you are having the following build failure, go to cozmo-game/unity/ios/ then throw the HockeyApp file in there into the trash and rebuild. Currently
delete does not properly remove the HockeySDK file and the link is broken.
    ⚠️  ld: directory not found for option '-F/Users/ryananderson/Desktop/cozmo-game/unity/ios/HockeyApp'
    
    ❌  ld: framework not found HockeySDK
    
    
    
    ❌  clang: error: linker command failed with exit code 1 (use -v to see invocation)
    
    
    ** BUILD FAILED **
    

### webots orphaned processes

 * Happens often when simulator crashes
 * `ps -ef | grep simulator\/controllers | cut -d ' ' -f 4 | xargs kill`
 * You can also search for orphaned processes in Activity Monitor.

### ios-deploy

To install, run or uninstall, you must have the ios-deploy application. To get it, run these commands:

    brew install node
    npm install -g ios-deploy

ios-deploy does not seem to be 100% reliable, nor does it have great error messages. If you have problems:

1. Make sure your ios device is connected and trusts the machine. If it's still waiting for a device, sometimes disconnecting and reconnecting helps.

2. Make sure you have the latest Xcode, or at least one that can build an iOS app for the targeted device.

3. If you have never built on this particular device, you may need to open the workspace in Xcode once with the device connected so that it can load some symbol files. (Unsure why this can't be done from the command line.)

4. When running, sometimes the lldb (debugger) will fail to attach and you won't get logs. In that case, the best you can do is rerun.

5. If none of the above apply, it's likely it won't work in Xcode either and Xcode has better error messages, so try running it from there.

6. ios-deploy is a popular enough program that google can usually lead you to an answer.

### standalone cozmo engine android apk

Refer to [standalone-apk/README.md](standalone-apk/README.md) for instructions on building and running the standalone cozmo engine android apk, the beginnings of Cozmo 2.0 engine
