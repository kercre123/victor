# Webots Simulator Setup

We are currently using [Webots version R2018a](https://www.cyberbotics.com/archive/mac/webots-R2018a.dmg). A license is required to run the Webots simulator. Submit a ticket to the Helpdesk to request a license. 

## Setup

Once Webots is installed, test it out by trying opening `simulator/worlds/bradsWorld.wbt`.

1. Go to the menu item Webots->Preferences. Select OpenGL and check the 'Disable shadows' box. Shadows can particularly interfere with the downward facing camera. We can assume this area will be actively lit on the robot.

1. Add `WEBOTS_HOME` environment variable by adding the following line to your `.bashrc` or `.bash_profile`:

        export WEBOTS_HOME=/Applications/Webots

    This is necessary to build controller or supervisor projects from the command line, which seems necessary when you have more than one controller or supervisor in your world.

1. Create a file called `console_vars.ini` in the directory `simulator/controllers/webotsCtrlGameEngine`. Add the following lines to the file:

        ; Console Var Ini Settings
        ; Edit Manually or Save/Load from Menu
        
        [Channels]
        unnamed = false
        
        [Firmware]
        SkipFirmwareAutoUpdate = true
        
        [Logging]
        EnableCladLogger = false

    This will prevent firmware from auto-updating when you connect to a physical robot with Webots. It also disables some default logging settings that can be problematic.

## Overview

Webots runs concurrent processes for each active simulation object via "controllers". Cozmo simulations consist of the following controllers which can be found in [simulator/controllers](./controllers). In order to debug with breakpoints you'll need to first attach to the appropriate process (In Xcode, go to Debug > Attach To Process).

* `webotsCtrlGameEngine` Engine
* `webotsCtrlRobot` Simulated Robot
* `webotsCtrlLightCube` Simulated LightCube
* `webotsCtrlViz` Visualization overlays and display windows
* `webotsCtrlKeyboard` Keyboard interface for communicating with engine in place of the Unity app. Useful for quick headless (i.e. no Unity UI/Game) development.
* `webotsCtrlDevLog` For visualizing logs recorded from an application run. (Only used in `devLogViz.wbt`)
* `webotsCtrlBuildServerTest` For Webots-based tests (that are executed on the build server with every PR and master build, as well as in nightly tests)

## Useful Worlds

The Webots 'world' files define the simulation environment, and can be found in [simulator/worlds](worlds). Here are some useful worlds for common development cases:

### 'Headless' development (without Unity)

These worlds can be controlled using the Webots Keyboard Controller, making them useful for testing locally without the overhead of having Unity running.

* `cozmoViz.wbt` (physical robot) Includes a keyboard controller and the engine. Prefer this world when testing with a physical robot.
* `bradsWorld.wbt` (simulated robot) Barebones world that includes a keyboard controller, simulated robot, and a few cubes.
* `raul_tableWithFloor.wbt` (simulated robot) More realistic world that contains a table and some common objects.

### Full App Simulation (with Unity)

These worlds are used in conjunction with Unity to provide a simulated app experience.

* `cozmoVizForUnity.wbt` (physical robot) Runs the engine. Prefer this world if you want to run Unity to talk to a physical robot.
* `PatternPlay.wbt` (simulated robot) Simulated environment for a virtual robot. Useful for using Unity to run simulated games. You can create your own versions of this ideal for the game you are testing.

### Others

* `remoteAnimationWorld.wbt` Used by the animation tool to see your animation on a simulated robot.
* `devLogViz.wbt` For visualizing logs recorded from an application run. Useful for debugging issues reported by QA. In the Webots tree view, look for WebotsDevLogController > logsDirectory. This field should be set to the absolute path of the un-tarred game log that you'd like to view.

## Troubleshooting

### Orphaned Processes

It's common to have orphaned webots controller processes hanging around after a simulation crashes. If you suspect that's the case, run

        ps -ef | grep simulator\/controllers | cut -d ' ' -f 4 | xargs kill
 
You can also search for orphaned processes in Activity Monitor.

### Considerations for Running with a Physical Robot

The 'pause' button does not really work with a physical robot, since the timing of robot<>engine messages is affected.

Also, keep in mind that once Webots connects to the robot, the connection will remain alive until the Webots world is reset with "Revert World" button. This may cause your robot to seem unresponsive if you have an open webots world that is still running or paused.

### Insufficient shared memory

At times, I have seen error messages to the effect of 'cannot allocate sufficient shared memory'.  There appears to be a memory leak of some kind in Webots and you may need to clear them out manually.

Use the command line tool ipcs to view what shared memory segments are currently allocated in the system. Run

    ipcs -p

to view the shared memory segments. Verify that these segments are not associated with a running process by making sure that the associated process IDs do not refer to an existing  process. You can do this with the command

    ps -ax | grep <CPID or LPID>

Once you have verified these are orphaned shared memory segments, you can remove them with the command 

    ipcrm  -m <shared memory ID>   (The shared memory ID is under the ID column of the ipcs output)


You should also remove any semaphores with

    ipcrm -s <semaphore ID>

If the shared memory segments are still lingering around and their KEY reads 0, the parent process needs to be killed. Run the command

    kill -9 <LPID>

where LPID is the LPID of the shared memory segment. It's probably identical for all the segments.
