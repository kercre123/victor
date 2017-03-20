# Cozmo 'standalone' apk

This is a standalone version of the Cozmo engine, meant to run on an Android device, but without Unity.

Steps:

1.  Follow these instructions to update `cozmoclad` from the `nextclad` branch:  
[Using local Cozmo app builds with the SDK](https://ankiinc.atlassian.net/wiki/spaces/COZMO/pages/122060851/Using+local+Cozmo+app+builds+with+the+SDK)
2.  Stop the regular Cozmo App from running on the Android device:  
    ```
    adb shell am force-stop com.anki.cozmo
    ```
3.  Build, install, and run on the Android device:  
    ```
    ./configure.py -p android run -l --features standalone
    ```
4.  Get the standalone engine to load assets and connect to a robot:  
    `./tools/sdk_devonly/start_sim.py -r`  
The -r means 'use a real robot, instead of webots simulator'.  (webots simulator integration may not work yet).
5.  Now you can run the example python scripts in [cozmo-python-sdk/examples/tutorials](https://github.com/anki/cozmo-python-sdk/tree/nextclad/examples)
6.  You can also use -f or --freeplay-mode as an argument to start_sim.py, to start in freeplay mode.
