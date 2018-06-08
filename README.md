# victor (aka Vector)

`victor` is the _code name_ for the next iteration of the Cozmo product line. This repo contains code for the embedded firmware (syscon), robotics, animation, engine, and app layers.

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
Update the submodules:

```
git submodule update --init --recursive
git submodule update --recursive
```

### Miscellaneous setup

1. To speed up builds, [install `ccache`](/docs/ccache.md) on your system and the build system will automatically start using it.

1. Add the following to your `~/.bash_profile`:

    ```
    source <PATH_TO_YOUR_VICTOR_REPO>/project/victor/scripts/usefulALiases.sh
    ```

    This will give you access to very useful aliases for the various build/deploy scripts. These aliases will be referenced extensively in this doc.

## Building Victor

If you're a developer, it is highly recommended that you check out the [Victor Build System Walkthrough](/docs/build-system-walkthrough.md) doc to familiarize yourself with the build system. The underlying build system for victor is [`CMake`](https://cmake.org/).  The appropriate version of CMake and other dependencies required for building victor will be fetched automatically.

1. To build for vicos (the default), ensure you are in the `victor` directory and run

    ```
    victor_build_debug
    ```

    If this command fails for some reason, try running `victor_build_debug -fX`<sup>1</sup>.
    
    Note if performance is a problem there is an option for a debug build with compiler optimizations enabled

    ```
    victor_build_debugo2
    ```

    Note that we often build in `release` mode, since `debug` mode is extremely slow on the robot. You can build in release by using `victor_build_release`.

    This differs from a shipping build as some developer tools are included, e.g. webserver, for a build that is shipped to consumers use

    ```
    victor_build_shipping
    ```

1. To build for mac, run

    ```
    project/victor/build-victor.sh -p mac
    ```

    If this command fails for some reason, try running `project/victor/build-victor.sh -p mac -fX`<sup>1</sup>.

1. If you want to force a 'clean' build, the brute-force method is to run

    ```
    git clean -dffx _build EXTERNALS generated
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

If your robot is >= 0.9.1 then it has a built-in ssh key. To interact with the robot you need to know its IP address and you must have the ssh key on your computer.

1. Download the private key to `~/.ssh/id_rsa_victor_shared`

    ```
    curl -sL -o ~/.ssh/id_rsa_victor_shared https://www.dropbox.com/s/mgxgdouo0id6j9m/id_rsa_victor_shared?dl=0
    chmod 600 ~/.ssh/id_rsa_victor_shared
    ssh-add -K ~/.ssh/id_rsa_victor_shared
    ```

    If you're on macOS Sierra you may want to add the `ssh-add` line to your `.bashrc` or `.bash_profile` since adding to keychain with `-K` is broken in Sierra.

1. Optionally create a configuration for your Victor robot in `~/.ssh/config`

    ```
    Host 192.168.42.*
      User root
      IdentityFile ~/.ssh/id_rsa_victor_shared
      StrictHostKeyChecking no
    ```

    If you still get an `ssh` error that says "Too many authentication failures", try adding `IdentitiesOnly=yes` to the above entry.

1. Build, deploy, and run commands as normal (See []()). You can specify the target robot with `-s` on most of the deploy and run commands or you can set the `ANKI_ROBOT_HOST` environment variable in your shell to a default host.

    ```
    export ANKI_ROBOT_HOST="<ip address>"
    ```

1. You can open an ssh session on the robot as follows:

    ```
    ssh root@${ANKI_ROBOT_HOST}
    ```

    or execute remote commands. e.g. `ssh root@${ANKI_ROBOT_HOST} "logcat -vthreadtime"`


### Connecting over WiFi

Follow the instructions at:

[Victor OTA udpate using Mac-Client tool](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/323321886/Victor+OTA+update+using+Mac-Client+tool#VictorOSand\OTAupdateusingMac-Clienttool-OTA) to setup WiFi on your robot.

### Getting Victor's IP address

1. Power on your robot and wait until the eyes appear on the screen.
1. Place robot on charger.
1. Double click his back button.
1. Raise the lift fully, then lower it fully.
1. The robot's IP address will appear in green if connected to a network. If the IP address appears in red, then the robot is not properly connected. Follow the steps [here](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/323321886/Victor+OTA+update+using+Mac-Client+tool#VictorOSand\OTAupdateusingMac-Clienttool-OTA) to get him connected to a network.

### Deploying to a particular robot

1. For convenience when working with multiple robots, or if you just want to be sure you're targeting a specific robot, almost all of the commands accept a robot IP with the -s option. For example:

    ```
    victor_deploy_debug -s 192.168.43.45
    ```

    will deploy the debug version to the robot at 192.168.43.45.

### Deploying binaries and assets to physical robot

1. Run `victor_stop` to stop the processes running on the robot.

1. Run `victor_deploy_debug` to deploy the binaries and asset files to the robot. To deploy the release version, use `victor_deploy_release`.
 
1. If the operation times out, power cycle the robot and try again.

## Running Victor

Once the binaries and assets are deployed, you can run everything by running `victor_start`.

You can view output from all processes by running `victor_log`. You can save the output to file while also viewing the live output by running `victor_log | tee log.txt`.

### Connect to Victor with Webots

Note that we have floating licenses for Webots. Check with your friendly producers to see if it's OK for you to install and use Webots, since you may be taking a license from someone who really needs it. 

1. First, make sure you have built for mac (see instructions above) on the same branch as the binaries on the physical robot.

1. Open the world [cozmo2Viz.wbt](/simulator/worlds/cozmo2Viz.wbt) in a text editor, and set the field `engineIP` to match your robot's IP address. 

1. Run the world to connect to the robot. The processes must be fully up and running before connecting with webots. If in doubt, stop the world and run `victor_restart` first.

### Running Victor in Webots (simulated robot)

See [simulator/README.md](/simulator/README.md).

## Updating syscon (body firmware)

Most developers shouldn't have to do this. If you think you need to update your syscon firmware, check with [Al](https://ankiinc.atlassian.net/wiki/display/~Al.Chaussee), [Kevin Y](https://ankiinc.atlassian.net/wiki/display/~kevin), or [Matt M](https://ankiinc.atlassian.net/wiki/display/~matt.michini).

## Having trouble?

Check out the [Frequently Asked Questions](/docs/FAQ.md) page. When you get new questions answered, consider adding them to the list to help others!

[Victor Build Setup](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/221151299/Victor+Build+Setup)

[Victor DVT3 ssh connection](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/368476326/Victor+DVT3+ssh+connection)

[Victor DVT3 OTA Instructions](https://ankiinc.atlassian.net/wiki/spaces/ATT/pages/323321886/Victor+OTA+update+using+Mac-Client+tool#VictorOSand\OTAupdateusingMac-Clienttool-OTA)

----
Copyright (C) 2018 Anki Inc.
