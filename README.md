cozmo-game
==================

The Cozmo repository for game prototyping.

Building cozmo-game
==========================

At this point, only Mac and iOS are supported. The engine itself supports more platforms. See [products-cozmo](https://github.com/anki/products-cozmo).

#### Prerequisites

1. You'll need [brew](http://brew.sh/) to install some tools.

2. Install CMAKE.
    brew install cmake

2. The simulation/visualization environment we use is [Webots](https://www.cyberbotics.com/overview). 

3. Move the OpenCV libraries inside of Webots out of the way:
    cd /Applications/Webots/lib
    mkdir opencv_unused
    mv libopencv* opencv_unused

#### Build coretech-external

Instructions [here](https://github.com/anki/coretech-external).
Important: Make sure you have your paths (eg. ~/.bash_profile) setup correctly.
Important: Once you set your paths you may need to restart your terminal for the settings to take effect.

#### Building cozmo-game

1. To run the configure python scripts you'll need xcpretty.
    sudo gem install xcpretty

2. Run ./configure

3. Run ./configure build

#### iOS Project

For working with iOS within cozmo-game, first do the above and then:

1. Build the Cozmo iOS fat libs by running this from the cozmo-game root directory:
  * python make_iOS_libs.py

2. Open the CozmoVision Xcode project found in the "cozmo-game/ios" directory.
  Experiment with the Webots world file: 
  * cozmo-game/robot/simulator/worlds/iOStest.wbt 

configure.py Commands
==========================

The basic commands are as follows:

    ./configure.py generate    # (default) create the Xcode projects and workspaces.
    ./configure.py build       # generate, then use xcodebuild to build the generated projects (including Unity on iOS).
    ./configure.py install     # generate, build, then install the app on a connected ios device.
    ./configure.py run         # generate, build, install, then debug the app on a connected ios device using lldb.
    ./configure.py uninstall   # uninstall the app from a connected device.
    ./configure.py clean       # use xcodebuild to clean the generated projects (assuming they exist).
    ./configure.py delete      # delete all generated projects and compiled files.
    ./configure.py wipeall!    # delete, then wipe all ignored files in the entire repository

Only generate and delete are really useful for C++ developers since the other commands are just duplicates of what you have in Xcode. C# developers might find the terminal commands easier to work with, and eventually the terminal versions should be cross-platform.

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

lldb Commands
==========================

 * Control-C seems to just crash the ios app without fully closing it.

 * Type `quit` or `exit` and hit enter, then `y` and hit enter to confirm.

 * ios-deploy also adds the `safequit` command, but sometimes it only kills the process and you have to `quit` afterward.

 * Note that all these commands work when logs are spamming, even if they get cut off.

 * Sometimes lldb hangs. In that case, use activity monitor to force quit lldb or open a new terminal and type `pkill lldb`.

 * Sometimes it hides console input after it closes. In that case, you can type `reset` and hit enter.

Here's a good link for basic commands: [I don't really want to learn lldb, I just want to fix a crash.](http://meowni.ca/posts/unscary-lldb/)


webots orphaned processes
==========================

 * happens often when simulator crashes
 * `ps -ef | grep simulator\/controllers | cut -d ' ' -f 4 | xargs kill`
