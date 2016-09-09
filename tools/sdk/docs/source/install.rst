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
* A smartphone with the Cozmo app installed, connected to the computer via USB cable

^^^^^^^
Install
^^^^^^^

.. important:: To ensure that your system is properly set up to run the Cozmo SDK, please ensure Homebrew and Python 3.5.2 (or later) are installed and the pip command is upgraded.

OSX: install `Homebrew <http://brew.sh>`_ on your system according to the latest instructions. Once Homebrew is installed, type the following into your Terminal window to install Python::

  brew install python3

Linux: type the following into your Terminal window to install Python::

  sudo apt-get install python3

OSX/Linux: upgrade pip by typing the following into the Terminal window::

  pip3 install -U pip

Windows: download the `executable file from Python.org <https://www.python.org/downloads/>`_ and run it on your computer. Once installed, upgrade pip by typing the following into the Command window::

  python -m pip install -U pip

..

To install the SDK, unzip the *cozmosdk.zip* file to the desired directory on your system.
