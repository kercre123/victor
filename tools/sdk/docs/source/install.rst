##################
Installation Guide
##################

.. warning:: THIS IS THE ALPHA BUILD OF THE COZMO SDK. All information within this SDK is subject to change without notice.

The Cozmo API is primarily self-contained within the Cozmo app. However, the mobile device the app is installed on must be tethered to a computer via USB cable in order to create new features or run new programs.

------------
Installation
------------

^^^^^^^^^^^^^
Prerequisites
^^^^^^^^^^^^^

* Python 3.5.2 or later
* WiFi connection
* An iOS or Android mobile device with the Cozmo app installed, connected to the computer via USB cable

^^^^^^^
Install
^^^^^^^

.. important:: To ensure that your system is properly set up to run the Cozmo SDK, please ensure Python 3.5.2 (or later) is installed and the pip command is upgraded.

"""
OSX
"""

Install `Homebrew <http://brew.sh>`_ on your system according to the latest instructions. Once Homebrew is installed, type the following into your Terminal window to install Python::

  brew install python3

Then upgrade pip by typing the following into the Terminal window::

    pip3 install -U pip

"""""
Linux
"""""

Type the following into your Terminal window to install Python::

  sudo apt-get install python3

Then install pip by typing in the following into the Terminal window::

  sudo apt install python3-pip

"""""""
Windows
"""""""

Download the `executable file from Python.org <https://www.python.org/downloads/>`_ and run it on your computer. Once installed, upgrade pip by typing the following into the Command window::

  python -m pip install -U pip

..

.. important:: Android Debug Bridge (adb) MUST be installed on your computer and USB debugging MUST be enabled in order to use the SDK on an Android device.

To install Android Debug Bridge on OSX, type the following into a Terminal window (requires Homebrew to be installed)::

    brew install android-platform-tools

"""""""
Install
"""""""

To install the SDK, unzip the *cozmosdk.zip* file to the desired directory on your system.


^^^^^^^^^^^^^^^^^^^^
Android Debug Bridge
^^^^^^^^^^^^^^^^^^^^

**To install Android Debug Bridge on Windows:**

1. Open your internet browser and go to `the Android developer website <https://developer.android.com/studio/index.html#Other>`_ .
2. Scroll down to *Get just the command line tools*. Download the SDK tools package.
3. If you downloaded the ``.zip`` file instead of the ``.exe`` file, unzip it into your chosen directory.
4. Run the installer to start the Android SDK Tools Setup Wizard.
5. The Setup Wizard will direct you to install the Java Development Kit (JDK) if you do not have it installed.
6. Complete installation of the Android SDK Tools. Take note of the directory it was installed to (e.g. C:\Program Files (x86)\Android).
7. In the Android SDK Tools location, run the SDK Manager as Administrator.

  a. Deselect everything except for *Android SDK Platform - tools*.
  b. Nexus phone users should also keep the *Google USB Driver* option selected in order to download Google USB drivers.
  c. Click **Install** once finished.
  d. Android Debug Bridge (adb) should now be installed to *platform-tools*.

8. Enable USB Debugging on your phone.

  a. To show Developer options, tap seven (7) times on the Build Number listed under *Settings -> About Phone*.
  b. Then, under *Settings -> Developer Options*, enable USB debugging.

9. Connect your iOS or Android device to your computer via USB. When the *Allow USB Debugging?* popup displays, tap **OK**.
10. Add adb to your PATH environment variable.

  a. Right-click the Start menu and select System.
  b. Select *Advanced System Settings -> Advanced -> Environment Variables*.
  c. Under *User Variables*, select *PATH* and click **Edit**.
  d. Under *Edit Environment Variables*, click **New** and add the path to adb (e.g. C:\Program Files (x86)\Android\android-sdk\platform-tools).

11. At the command line, type this command to confirm that your device shows::

      adb devices

..

At least one device should show in the result, for example::

    List of devices attached
    88148a08    device

..

**To install Android Debug Bridge on Linux:**

1. If you do not yet have the Java Development Kit (JDK) installed, you must install it.

  a. To check to see if you have java installed, enter the following command into the Terminal::

        java -version

  b. If java is not installed, install it with the following command::

        sudo apt install default-jre

2. Open your internet browser and go to `the Android developer website <https://developer.android.com/studio/index.html#Other>`_ .
3. Scroll down to *Get just the command line tools*. Download the SDK tools package.
4. Unzip the file into your chosen directory.
5. In the downloaded Linux SDK tools, start the Android SDK Manager by executing the program **android** in *android-sdk/linux/tools* like this::

        cd YOUR_ANDROID_SDK_LOCATION/android-sdk/linux/tools
        ./android

6. Perform the following steps in the Android SDK Manager.

  a. Deselect everything except for *Android SDK Platform - tools*.
  b. Nexus phone users should also keep the *Google USB Driver* option selected in order to download Google USB drivers.
  c. Click **Install** once finished.
  d. Android Debug Bridge (adb) should now be installed to *platform-tools*.

7. Add adb to your PATH.

  a. Edit your `~/.bashrc` file and add this line::

        export PATH=${PATH}:YOUR_ANDROID_SDK_LOCATION/android-sdk-linux/platform-tools

  b. Save `.bashrc` and then call::

        source .bashrc

  c. Confirm that adb is in your PATH by calling the following command::

        whereis adb

  d. The result of this command should be::

        adb: YOUR_ANDROID_SDK_LOCATION/android-sdk-linux/platform-tools/adb

8. Make sure USB Debugging on your phone is enabled. To show the Developer options, tap 7 times on Settings > About phone > Build number. Then, under Settings > Developer Options, enable USB debugging.
9. After connecting the phone to the computer via USB, in the “Allow USB Debugging?” popup, tap OK.
10. At the command line, type this command to confirm that your device shows::

      adb devices

..

At least one device should show in the result, for example::

    List of devices attached
    88148a08    device
