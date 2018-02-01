## FAQ

If you have a question that you get answered (e.g. in a Slack channel) which might plague others, consider creating an entry here for it.

* How do I set a console var fromm Webots?
  - Set the `consoleVarName` and `consoleVarValue` strings under the WebotsKeyboardController in the scene tree at left. Press `]` (closing square bracket) to send the message to the engine to set the console var. 
  - Using `}` (closing curly brace) instead will use the name and value strings as a console _function_ and its arguments instead.
  - Hold down `ALT` to send either of the above to the animation process to set console vars/functions there.

* How do I access the Victor embedded web server, from which I can set console vars?
  - [Run the Victor web server](/docs/development/web-server.md) 
  
* `victor_stop`/`victor_restart` hangs with `stopping <number>...` when trying to kill one of the processes
  - Manually kill the process with `adb shell kill -9 <number>`
  - Run `victor_stop` to ensure the rest of the processes are stopped
  
* One of the processes crashed/aborted and the stack trace has no symbols, how do I get symbols for the trace?
  - Manually deploy the full libraries (with symbols) to the robot; they are located in `_build/android/<Debug/Release>/lib/*.so.full`
  - `adb push _build/android/<Debug/Release>/lib/<one_or_more_of_the_libraries>.so.full /data/data/com.anki.cozmoengine/lib/<one_or_more_of_the_libraries>.so`
  - Reproduce the crash and you should now see symbols
  
* How do I do performance analysis/benchmarking on robot?
  - Use simpleperf to generate list of highest overhead functions in a process by running `project/victor/simpleperf/HOW-simpleperf.sh`
  - Use inferno to generate a flame graph of call hierarchies by running `project/victor/simpleperf/HOW-inferno.sh`
  - By default both scripts run on the engine process, `cozmoengined`. Change this by prepending `ANKI_PROFILE_PROCNAME="<name_of_process>"` to the command to run the script. The other two process names are `victor_animator` and `robot_supervisor`
  - To see overall cpu load per process run `top -m 10`
  
* Victor won't turn on, the top backpack light blinks orange/red
  - Victor has low battery, turn Victor off and turn back on with the backpack button. Put Victor on the charger and the lights should return to normal multicolor or green, leave on charger for ~30 minutes
  
* `Sending Mode Change ...` or `spine_header ...` errors continually spam in the logs
  - Ensure you have deployed correctly built code otherwise Syscon and robot process are likely incompatible, give to Al or Kevin to have firmware flashed
  
* I can `adb connect` to Victor and ping it but I can't deploy or access the shell
  - Run `adb devices`, make sure only one device, Victor, is listed
  - If Victor is listed as being `offline`, power cycle the robot. Multiple devices might have been connected or adb got into a bad state possibly due to the network connection changing while you were connected
  - Another solution to Victor being `offline` is to connect with the `victor-ble-cli` tool and run `restart-adb` (you'll have to run `adb connect` again on your computer)
  
* I can't connect or ping Victor, he doesn't seem to be connected to wifi
  - Use the [BLE-CLI](../tools/victor-ble-cli) tool to connect over BLE and configure wifi
    Modify `vic_join_wifi.sh`, replace the `elif [ "K" ...` block in the `robot configs` section with your robot's `robotname` (serial number). Run the script with `./vic_join_wifi.sh K` to reconfigure your robot to connect to the AnkiRobits network.
  - This connection will likely only be temporary, if after power cycling your robot is again not autoconnecting then bring to Al or Nathan
  
* I get `Failed to connect to non-global ctrl_ifname: wlan0  error: No such file or directory` errors in the [BLE-CLI](../tools/victor-ble-cli) tool
  - Trying restarting the [wpa_supplicant](https://en.wikipedia.org/wiki/Wpa_supplicant)
  - `pkill wpa_supplicant` then `wpa_supplicant -iwlan0 -Dnl80211 -c/data/misc/wifi/wpa_supplicant.conf -O/data/misc/wifi/sockets -B`

* Seriously... I cannot get connected to my home WIFI using wifi-set-config ^^
  - These issues should be resolved when we update the OS, but in the meantime:
  - You can connect with a static IP to your network using a shell script located at .../victor/tools/victor-ble-cli/vic_join_network.sh
  - You will have to rerun the script every time you power cycle so it may be worth your time to make an alias with your network config as arguments

* Deploying is super slow
  - Try turning off wifi on your laptop and use ethernet instead
  - Run `victor_stop` before deploying
  - Try moving to somewhere else in the office, a conference room or the kitchen. Might be an access point/network problem
  - If nothing seems to help consider bringing to HW team, could be a bad antenna
  
* Deploying to the robot fails (possibly complains about out of disk space?)
  - You can safely remove the cache and output folders with `adb shell rm -rf /data/data/com.anki.cozmoengine/cache /data/data/com.anki.cozmoengine/files/output`
  - If this isn't enough, in `adb shell` you can check disk usage with `df` to list percentage of space each section has free, you should only care about the `/data` section. The percentage indicates how much space the section is using out of how much total space it has.
  - You can then inspect space usage within `/data` with `du -h /data` which will list disk usage of all directories in `/data`.    Everything that gets deployed lives in `/data/data/com.anki.cozmoengine`
  
* How do I run unit tests
  - https://ankiinc.atlassian.net/wiki/spaces/VD/pages/149363555/Victor+Unit+Tests

* I get permission denied during build `error: can't exec 'victor/_build/mac/Debug-Xcode/launch-c' (Permission denied)`
  - `chmod u=rwx victor/_build/mac/Debug-Xcode/launch-c`
  - `chmod u=rwx victor/_build/mac/Debug-Xcode/launch-cxx`

* To check the battery level:
  - With the processes running, press the backpack button twice. The battery voltage is in the bottom right. It should be around 4.0v. The backpack lights will blink green when charging (and the processes are running)
  - Otherwise: `adb shell cat /sys/devices/soc.0/qpnp-linear-charger-8/power_supply/battery/voltage_now`
  
* Webots Firewall Connection issues?
  - [Create a code signing certificate](/project/build-scripts/webots/FirewallCertificateInstructions.md)
  - [Run a sample script to initially sign](/simulator/README.md#firewall)

* Can't generate Xcode project using `build-victor.sh -p mac -g Xcode -C` and getting error output like this
```
-- The C compiler identification is unknown
-- The CXX compiler identification is unknown
CMake Error at CMakeLists.txt:9 (project):
  No CMAKE_C_COMPILER could be found.
CMake Error at CMakeLists.txt:9 (project):
  No CMAKE_CXX_COMPILER could be found.
```

  * ... then perform these steps (continued from above)
   	* Install command line tools
    * `sudo xcode-select -s /Applications/Xcode.app/Contents/Developer`# New Document
