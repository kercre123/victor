##################
Installation Guide
##################

.. warning:: THIS IS THE ALPHA BUILD OF THE COZMO SDK. All information within this SDK is subject to change without notice.

The Cozmo API is primarily self-contained within the Cozmo app. However, the smartphone the app is installed on must be tethered to a computer via USB cable in order to create new features or run new programs.

------------
Installation
------------

^^^^^^^^^^^^^
Prerequisites
^^^^^^^^^^^^^

* Python 3.5.2 or later
* WiFi connection
* An iOS or Android smartphone with the Cozmo app installed, connected to the computer via USB cable

^^^^^^^
Install
^^^^^^^

.. important:: To ensure that your system is properly set up to run the Cozmo SDK, please ensure Python 3.5.2 (or later) is installed and the pip command is upgraded.

OSX: install `Homebrew <http://brew.sh>`_ on your system according to the latest instructions. Once Homebrew is installed, type the following into your Terminal window to install Python::

  brew install python3

Linux: type the following into your Terminal window to install Python::

  sudo apt-get install python3

OSX/Linux: upgrade pip by typing the following into the Terminal window::

  pip3 install -U pip

Windows: download the `executable file from Python.org <https://www.python.org/downloads/>`_ and run it on your computer. Once installed, upgrade pip by typing the following into the Command window::

  python -m pip install -U pip

..

.. important:: Android Debug Bridge (adb) MUST be installed on your computer and USB debugging MUST be enabled in order to use the SDK on an Android device.

To install Android Debug Bridge on iOS, type the following into a Terminal window (requires Homebrew to be installed)::

    brew install android-platform-tools

..

To install Android Debug Bridge on Windows:

1. Open your internet browser and go to `the Android developer website <https://developer.android.com/studio/index.html#Other>`_ .
2. Scroll down to *Get just the command line tools*. Download the SDK tools package.
3. If you downloaded the ``.zip`` file instead of the ``.exe`` file, unzip it into your chosen directory.
4. Run the installer to start the Android SDK Tools Setup Wizard.
5. The Setup Wizard will direct you to install the Java Development Kit (JDK) if you do not have it installed.
6. Complete installation of the Android SDK Tools. Take note of the directory it was installed to (e.g. C:\Program Files (x86)\Android).
7. In the Android SDK Tools location, run the SDK Manager as Administrator.

  1. Deselect everything except for *Android SDK Platform - tools*.
  2. Nexus phone users should also keep the *Google USB Driver* option selected in order to download Google USB drivers.
  3. Click **Install** once finished.
  4. Android Debug Bridge (adb) should not be installed to *platform-tools*.

8. Enable USB Debugging on your phone.

  1. To show Developer options, tap seven (7) times on the Build Number listed under *Settings -> About Phone*.
  2. Then, under *Settings -> Developer Options*, enable USB debugging.

9. Connect your phone to your computer via USB. When the *Allow USB Debugging?* popup displays, tap **OK**.
10. Add adb to your PATH environment variable.

  1. Right-click the Start menu and select System.
  2. Select *Advanced System Settings -> Advanced -> Environment Variables*.
  3. Under *User Variables*, select *PATH* and click **Edit**.
  4. Under *Edit Environment Variables*, click **New** and add the path to adb (e.g. C:\Program Files (x86)\Android\android-sdk\platform-tools).

..

To install the SDK, unzip the *cozmosdk.zip* file to the desired directory on your system.
