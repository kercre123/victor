##################
Installation Guide
##################

.. warning:: THIS IS THE ALPHA BUILD OF THE COZMO SDK. All information within this SDK is subject to change without notice.

To use the Cozmo SDK, the Cozmo mobile app must be installed on your mobile device and that device must be tethered to a computer via USB cable.

------------
Installation
------------

^^^^^^^^^^^^^
Prerequisites
^^^^^^^^^^^^^

* Python 3.5.1 or later
* WiFi connection
* An iOS or Android mobile device with the Cozmo app installed, connected to the computer via USB cable

^^^^^^^
Install
^^^^^^^

.. important:: To ensure that your system is properly set up to run the Cozmo SDK, please ensure Python 3.5.1 (or later) is installed and the pip command is upgraded.

""""
OS X
""""

.. important:: Users that do not have root privileges on the system can use ``pip install --user`` to bypass that requirement.

1. Install `Homebrew <http://brew.sh>`_ on your system according to the latest instructions. Once Homebrew is installed, type the following into your Terminal window to install Python::

    brew install python3

2. Then upgrade pip by typing the following into the Terminal window::

    pip3 install -U pip

3. (RECOMMENDED) Once the SDK is installed, type the following to install camera image processing support::

    pip3 install --user cozmo-0.6.3a35-py3-none-any.whl[camera]


"""""""""""""""""""""""""""""
Linux (Ubuntu 14.04 or 16.04)
"""""""""""""""""""""""""""""

The Cozmo SDK is tested and and supported on Ubuntu 14.04 and above. While the SDK is not guaranteed to work on other versions of Linux, please ensure the following dependencies are installed if you wish to run the SDK on any other Linux system:

  * Python 3.5.1
  * pip for Python 3 (Python package installer)
  * Android command line tools (https://developer.android.com/studio/index.html#Other)
  * usbmuxd for iOS

.. important:: Users that do not have root privileges on the system can use ``pip install --user`` to bypass that requirement.

**For version 14.04 only:**

1. Type the following into your Terminal window to install Python 3.5::

    sudo add-apt-repository ppa:fkrull/deadsnakes
    sudo apt-get update
    sudo apt-get install python3.5
    sudo update-alternatives --install /usr/bin/python3 python3.5 /usr/bin/python3.5.1

2. Then install pip by typing in the following into the Terminal window::

    sudo apt-get install python3-setuptools
    sudo easy_install3 pip

3. Last, install Tkinter::

    sudo apt-get install python3.5-tk

**For Ubuntu 16.04:**

1. Type the following into your Terminal window to install Python::

    sudo apt-get install python3

2. Then install pip by typing in the following into the Terminal window::

    sudo apt install python3-pip

3. Last, install Tkinter::

    sudo apt-get install python3-pil.imagetk

**For both versions of Ubuntu:**

4. (RECOMMENDED) Once the SDK is installed, type the following to install camera image processing support::

    pip3 install --user cozmo-0.6.3a35-py3-none-any.whl[camera]


"""""""
Windows
"""""""

.. important:: Users that do not have administrative privileges on the system can use ``pip install --user`` to bypass that requirement.

1. Download the `executable file from Python.org <https://www.python.org/downloads/>`_ and run it on your computer. Once installed, upgrade pip by typing the following into the Command window::

    python -m pip install -U pip

..

2. (RECOMMENDED) Once the SDK is installed, type the following to install camera image processing support::

    pip3 install --user cozmo-0.6.3a35-py3-none-any.whl[camera]

..

.. important:: Android Debug Bridge (adb) MUST be installed on your computer and USB debugging MUST be enabled in order to use the SDK on an Android device.


"""""""
Install
"""""""

To install the SDK, download the 'cozmo-0.6.3a35-py3-none-any.whl <http://cozmosdk.anki.com/sdkbeta/0.6.3a35/cozmo-0.6.3a35-py3-none-any.whl>'_ type the following into your Command-line/Terminal window::

    pip3 install --user cozmo-0.6.3a35-py3-none-any.whl

Example programs are available for `OS X/Linux <http://cozmosdk.anki.com/sdkbeta/0.6.3a35/sdk-examples-0.6.3a35.tar.gz>`_ and `Windows <http://cozmosdk.anki.com/sdkbeta/0.6.3a35/sdk-examples-0.6.3a35.zip>`_ platforms. Once downloaded, extract these packaged files to the SDK directory.

--------------------
Android Debug Bridge
--------------------

**To install Android Debug Bridge on OS X:**

Type the following into a Terminal window (requires Homebrew to be installed)::

    brew install android-platform-tools

Then enable USB Debugging on your mobile device.

    On Android devices:

      1. Tap seven (7) times on the Build Number listed under *Settings -> About Phone*.
      2. Then, under *Settings -> Developer Options*, enable USB debugging.

    On Amazon Kindle Fire:

      1. Tap seven (7) times on the Serial Number listed under *Settings -> Device Options*.
      2. Then, under *Settings -> Device Options -> Developer Options*, turn on Enable ADB.

..

**To install Android Debug Bridge (adb) on Windows:**

1. Open your internet browser and go to `the Android developer website <https://developer.android.com/studio/index.html#Other>`_ .
2. Scroll down to *Get just the command line tools*. Download the SDK tools package.
3. If you downloaded the ``.zip`` file instead of the ``.exe`` file, unzip it into your chosen directory.
4. Run the installer to start the Android SDK Tools Setup Wizard.
5. The Setup Wizard will direct you to install the Java Development Kit (JDK) if you do not have it installed.
6. Complete installation of the Android SDK Tools. Take note of the directory it was installed to (e.g. *C:\Program Files (x86)\Android*).
7. In the Android SDK Tools location, run the SDK Manager as Administrator.

  a. Deselect everything except for *Android SDK Platform - tools*.
  b. Click **Install** once finished.
  c. adb should now be installed to *platform-tools*.

8. Enable USB Debugging on your phone.

  a. On Android devices:

    1. Tap seven (7) times on the Build Number listed under *Settings -> About Phone*.
    2. Then, under *Settings -> Developer Options*, enable USB debugging.

  b. On Amazon Kindle Fire:

    1. Tap seven (7) times on the Serial Number listed under *Settings -> Device Options*.
    2. Then, under *Settings -> Device Options -> Developer Options*, turn on Enable ADB.

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

1. If you do not yet have the Java Development Kit (JDK) version 8 installed, you must install it.

  a. To check to see if you have java installed, enter the following command into the Terminal::

        java -version

  b. If JDK version 8 is not installed, install it with the following command:

    1. On Ubuntu version 14.04::

        sudo add-apt-repository ppa:webupd8team/java
        sudo apt-get update
        sudo apt-get install oracle-java8-installer

    2. On Ubuntu 16.04::

        sudo apt install default-jre

2. Open your internet browser and go to `the Android developer website <https://developer.android.com/studio/index.html#Other>`_ .
3. Scroll down to *Get just the command line tools*. Download the SDK tools package.
4. Unzip the file into your chosen directory.
5. In the downloaded Linux SDK tools, start the Android SDK Manager by executing the program **android** in *android-sdk-linux/tools* like this::

        cd YOUR_ANDROID_SDK_LOCATION/android-sdk-linux/tools
        ./android

6. Perform the following steps in the Android SDK Manager.

  a. Deselect everything except for *Android SDK Platform - tools*.
  b. Click **Install** once finished.
  c. Android Debug Bridge (adb) should now be installed to *YOUR_ANDROID_SDK_LOCATION/android-sdk-linux/platform-tools*.

7. Add adb to your PATH.

  a. Edit your `~/.bashrc` file and add this line::

        export PATH=${PATH}:YOUR_ANDROID_SDK_LOCATION/android-sdk-linux/platform-tools

  b. Save `.bashrc` and then call::

        source .bashrc

  c. Confirm that adb is in your PATH by calling the following command::

        which YOUR_ANDROID_SDK_LOCATION/android-sdk-linux/platform-tools/adb

  d. The result of this command should be::

        adb: YOUR_ANDROID_SDK_LOCATION/android-sdk-linux/platform-tools/adb

8. Enable USB Debugging on your phone.

  a. On Android devices:

      1. Tap seven (7) times on the Build Number listed under *Settings -> About Phone*.
      2. Then, under *Settings -> Developer Options*, enable USB debugging.

  b. On Amazon Kindle Fire:

      1. Tap seven (7) times on the Serial Number listed under *Settings -> Device Options*.
      2. Then, under *Settings -> Device Options -> Developer Options*, turn on Enable ADB.

9. After connecting the phone to the computer via USB, in the “Allow USB Debugging?” popup, tap OK.
10. At the command line, type this command to confirm that your device shows::

      adb devices

..

At least one device should show in the result, for example::

    List of devices attached
    88148a08    device
