# cozmo-game

Cozmo unity gameplay code. Also includes the current animation tool in /animation-tool/

Please read the [Unity Coding Guidelines](https://github.com/anki/cozmo-game/wiki/Unity-Coding-Guidelines).

# Building cozmo-game

At this point, only Mac and iOS are supported. The engine itself supports more platforms. See [products-cozmo](https://github.com/anki/products-cozmo).

### Prerequisites

Install CMAKE using [brew](http://brew.sh/).

    brew install cmake

The robotics simulation environment we use is Webots. We are currently using [version 8.2.1](https://www.cyberbotics.com/archive/mac/).

Move the OpenCV libraries inside of Webots out of the way:

    cd /Applications/Webots/lib
    mkdir opencv_unused
    mv libopencv* opencv_unused

### Build coretech-external

Instructions [here](https://github.com/anki/coretech-external).

Make sure you have your paths (eg. ~/.bash_profile) setup correctly it must include the line:

    export CORETECH_EXTERNAL_DIR="$HOME/coretech-external"

Once you set your paths you need to restart your terminal for the settings to take effect. Alternatively you can refresh your environment:

source ~/.bash_profile

### Building cozmo-game

To run the configure python scripts you'll need xcpretty.

    sudo gem install xcpretty

Run ./configure.py from the root of your cosmo-game folder.

    cd <path-to-cozmo-repository>
    ./configure.py
    ./configure.py build

### Webots

The Webots worlds can be found in cozmo-game/simulator/worlds

Useful worlds:

 * cozmoViz.wbt - Includes a keyboard controller and the engine. Used to talk to a physical robot.

 * cozmoVizForunity.wbt - runs the engine. Use this if you want to run Unity to talk to a physical robot.

 * PatternPlay.wbt - Simulated environment for a virtual robot. Useful for using Unity to run simulated games. You can create your own versions of this ideal for the game you are testing.

 * remoteAnimationWorld.wbt - Used by the animation tool to see your animation on a simulated robot.

### Unity

We are using [Unity 5.2.2f1](http://unity3d.com/get-unity/download/archive).

A dev scene that includes a simple list of challenges for Cozmo. Used in a development environment.

    <path-to-cozmo-repository>/Assets/Scenes/Dev.unity

The production scene used for when we deploy. This is where the final experience will live.

    <path-to-cozmo-repository>/Assets/Scenes/HubWorld.unity


# Optional Info

### configure.py Commands

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

### lldb Commands

 * Control-C seems to just crash the ios app without fully closing it.

 * Type `quit` or `exit` and hit enter, then `y` and hit enter to confirm.

 * ios-deploy also adds the `safequit` command, but sometimes it only kills the process and you have to `quit` afterward.

 * Note that all these commands work when logs are spamming, even if they get cut off.

 * Sometimes lldb hangs. In that case, use activity monitor to force quit lldb or open a new terminal and type `pkill lldb`.

 * Sometimes it hides console input after it closes. In that case, you can type `reset` and hit enter.

Here's a good link for basic commands: [I don't really want to learn lldb, I just want to fix a crash.](http://meowni.ca/posts/unscary-lldb/)

### webots orphaned processes

 * happens often when simulator crashes
 * `ps -ef | grep simulator\/controllers | cut -d ' ' -f 4 | xargs kill`
