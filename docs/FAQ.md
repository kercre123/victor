## FAQ

If you have a question that you get answered (e.g. in a Slack channel) which might plague others, consider creating an entry here for it.

* How do I set my Android NDK?
  - Follow the setup instructions for `adb` in [README.md](/README.md) to get the default Android NDK.
  - Don't try to override the default NDK unless you know what you are doing!

* How do I override the default NDK?
  - If you set `${ANDROID_NDK}` in your environment, the build script ([`build-victor.sh`](/project/victor/build-victor.sh)) will use it.
  - If you have `ndk.dir` set in your top-level `local.properties`, the environment script ([`envsetup.sh`](/project/victor/envsetup.sh)) will use it.
  - If you have an NDK installed by Android Studio, the android locator script ([`android.py`](/tools/build/tools/ankibuild/android.py)) will use it.
  - If you have an NDK set in your top-level `.buckconfig`, the android locator script ([`android.py`](/tools/build/tools/ankibuild/android.py)) will use it.

* How do I fix my Android NDK?
  - Check for each of the overrides listed above.
  - Figure out which one is wrong.
  - Fix it or remove it.
  - If you change your NDK settings, you must force a full rebuild by removing your top-level `_build/android`.

* How do I set a console var from Webots?
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
  - Use simpleperf to generate list of highest overhead functions in a process by running [`project/victor/simpleperf/HOW-simpleperf.sh`](/project/victor/simpleperf/HOW-simpleperf.sh)
  - If the script fails with the error: `[native_lib_dir] "./project/victor/simpleperf/symbol_cache" is not a dir`, cd into `./project/victor/simpleperf/` and run `make_symbol_cache.sh`
  - Use inferno to generate a flame graph of call hierarchies by running [`project/victor/simpleperf/HOW-inferno.sh`](/project/victor/simpleperf/HOW-inferno.sh)
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
  - Reboot the robot and hope!

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
  - Otherwise: `adb shell cat /sys/devices/soc/qpnp-linear-charger-8/power_supply/battery/voltage_now`
  
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
   
* Can't generate Xcode project using `build-victor.sh -g Xcode -C` and getting error output like this
```
CMake Error at lib/util/source/3rd/civetweb/cmake/FindWinSock.cmake:77 (if):
  if given arguments:

    "x86_64" "STREQUAL" "AMD64" "AND" "EQUAL" "4"

  Unknown arguments specified
Call Stack (most recent call first):
  lib/util/source/3rd/civetweb/src/CMakeLists.txt:26 (find_package)
```

  * you forgot `-p mac`
  
* My Victor won't turn on/stay on
  - If only the top backpack light blinks when on the charger then the robot is low on battery and will not turn on by just being placed on the charger
  - First turn the robot on with the backpack button and all the lights should turn on as normal
  - Quickly place the robot on the charger. It should stay on at this point and begin charging
  - Leave the robot on the charger for ~30 minutes

* When profiling I see "...doesn't contain symbol table"
  - This is just a warning, there are no symbol tables for the .so files on the device, instead we use symbols from the symbol cache

* How do I decipher this crash in the victor log

```
12-31 16:22:17.366  2534  2733 F libc    : Fatal signal 11 (SIGSEGV), code 1, fault addr 0x0 in tid 2733 (CozmoRunner)
12-31 16:22:17.367  1848  1848 W         : debuggerd: handling request: pid=2534 uid=0 gid=0 tid=2733
12-31 16:22:17.413  2884  2884 E         : debuggerd: Unable to connect to activity manager (connect failed: No such file or directory)
12-31 16:22:17.465  2884  2884 F DEBUG   : *** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***
12-31 16:22:17.465  2884  2884 F DEBUG   : Build fingerprint: 'qcom/msm8909/msm8909:7.1.1/NMF26F/andbui11141045:eng/test-keys'
12-31 16:22:17.465  2884  2884 F DEBUG   : Revision: '0'
12-31 16:22:17.466  2884  2884 F DEBUG   : ABI: 'arm'
12-31 16:22:17.466  2884  2884 F DEBUG   : pid: 2534, tid: 2733, name: CozmoRunner  >>> /data/data/com.anki.cozmoengine/bin/cozmoengined <<<
12-31 16:22:17.467  2884  2884 F DEBUG   : signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x0
12-31 16:22:17.467  2884  2884 F DEBUG   :     r0 ffffffff  r1 ffffffff  r2 00000001  r3 00000051
12-31 16:22:17.468  2884  2884 F DEBUG   :     r4 add70e80  r5 00000000  r6 00000000  r7 ae67f1a0
12-31 16:22:17.468  2884  2884 F DEBUG   :     r8 00000000  r9 aeaf4760  sl add780d0  fp b1938690
12-31 16:22:17.468  2884  2884 F DEBUG   :     ip b0b75870  sp ae67f158  lr afd6e92b  pc afd6e960  cpsr a00f0030
12-31 16:22:17.512  2884  2884 F DEBUG   : 
12-31 16:22:17.512  2884  2884 F DEBUG   : backtrace:
12-31 16:22:17.512  2884  2884 F DEBUG   :     #00 pc 00287960  /data/data/com.anki.cozmoengine/lib/libcozmo_engine.so
12-31 16:22:17.513  2884  2884 F DEBUG   :     #01 pc 002c34a7  /data/data/com.anki.cozmoengine/lib/libcozmo_engine.so
12-31 16:22:17.513  2884  2884 F DEBUG   :     #02 pc 00273793  /data/data/com.anki.cozmoengine/lib/libcozmo_engine.so
12-31 16:22:17.513  2884  2884 F DEBUG   :     #03 pc 002751ed  /data/data/com.anki.cozmoengine/lib/libcozmo_engine.so
```

Looking at the first four lines after `backtrace:` you can see the top four addresses on the stack and the shared library they reside.

  - `victor_addr2line libcozmo_engine.so 00287960 002c34a7 00273793 002751ed`

to get:

```
0x00287960: .anki/android/ndk-repository/android-ndk-r15b/sources/cxx-stl/llvm-libc++/include/memory:4041
0x002c34a7: projects/victor/_build/android/Release/../../../engine/aiComponent/behaviorComponent/behaviors/iCozmoBehavior.cpp:714
0x00273793: projects/victor/_build/android/Release/../../../engine/aiComponent/behaviorComponent/behaviorStack.cpp:123
0x002751ed: projects/victor/_build/android/Release/../../../engine/aiComponent/behaviorComponent/behaviorSystemManager.cpp:154
```
