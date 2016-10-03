#############
Vagrant Setup
#############

.. warning:: THIS IS THE ALPHA BUILD OF THE COZMO SDK. All information within this SDK is subject to change without notice.

The Cozmo API is primarily self-contained within the Cozmo app. However, the mobile device the app is installed on must be tethered to a computer via USB cable in order to create new features or run new programs.

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

1. Install the necessary prerequisites for your primary OS on your computer as shown in the Installation Guide.
2. If your mobile device runs Android, install Android Debug Bridge (adb) using the instructions in the Installation Guide.
3. Install Vagrant on your computer.

  a. Install `VirtualBox and the VirtualBox Extension Pack <https://www.virtualbox.org/wiki/Downloads>`_ from the official webpage. Both of these *must* be installed in order for Vagrant to work properly.
  b. Install `Vagrant <https://www.vagrantup.com/downloads.html>`_ from the official webpage.

3. Install the Cozmo app on your mobile device.
4. Plug the mobile device containing the Cozmo app into your computer.
5. Make sure Cozmo is powered on and charged. Connect to the Cozmo robot's WiFi from the mobile device.
6. Enter SDK mode on the app.
7. Connect to the Cozmo robot within the app.
8. For **Windows**:

  a. Download ``vagrant-bundle.zip`` from the `ANKI SDK website <http://developer.anki.com/>`_ .
  b. Unzip the ``vagrant-bundle.zip`` file into a folder of your choosing.
  c. Open a Command-line window.

    1. Enter the following command::

        cd (location you extracted the .zip file)

    2. Enter the following command::

        vagrant up

    .. important:: Wait for `vagrant up` to completely finish before continuing to the next step.

  d. Open the Virtual Machine.
  e. Within the VM, open xterm. Once xterm is open, run the following commands in order (wait for each command to finish before entering the next)::

      /vagrant/setup-vm.sh

    .. important:: Be sure to accept the Android SDK license prompt before continuing.

    .. code-block:: python

      cd /vagrant/examples

..

8. For **OS X/Linux**:

  a. Download ``vagrant-bundle.tgz`` from the `ANKI SDK website <http://developer.anki.com/>`_ .
  c. Open a Terminal window.

    1. Enter the following commands::

        cd (location you downloaded the vagrant-bundle.tgz file)
        tar -xzf vagrant-bundle.tgz

    2. Enter the following command::

        vagrant up

    .. important:: Wait for `vagrant up` to completely finish before continuing to the next step.

  d. Open the Virtual Machine.
  e. Within the VM, open xterm. Once xterm is open, run the following commands in order (wait for each command to finish before entering the next)::

        /vagrant/setup-vm.sh

    .. important:: Be sure to accept the Android SDK license prompt before continuing.

    .. code-block:: python

        cd /vagrant/examples

9. On the Cozmo app, swipe right on the gear icon at the top right corner to open the Settings menu.
10. Swipe left to show the Cozmo SDK option and tap the Enable SDK button.
11. To run a program enter the following ::

        python3 ./program_name.py
