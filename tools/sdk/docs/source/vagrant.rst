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

* WiFi connection
* An iOS or Android mobile device with the Cozmo app installed, connected to the computer via USB cable

^^^^^^^
Install
^^^^^^^

1. Install `VirtualBox and the VirtualBox Extension Pack <https://www.virtualbox.org/wiki/Downloads>`_ from the official webpage. Both of these *must* be installed in order for Vagrant to work properly.
2. Install `Vagrant <https://www.vagrantup.com/downloads.html>`_ from the official webpage.
3. Install the Cozmo app on your mobile device.
4. Plug the mobile device containing the Cozmo app into your computer.
5. For **Windows**:

  a. Download ``vagrant-bundle.zip`` from the `Anki SDK website <http://cozmosdk.anki.com/>`_ .
  b. Unzip the ``vagrant-bundle.zip`` file into a folder of your choosing.
  c. Open a Command-line window.

    1. Enter the following commands::

        cd (location you extracted the .zip file)
        cd vagrant-bundle

    2. Enter the following command::

        vagrant up

    .. important:: Wait for `vagrant up` to completely finish before continuing to the next step.

  d. Navigate to where the Virtual Machine is currently running.
  e. Within the VM, open xterm. Once xterm is open, run the following commands in order (wait for each command to finish before entering the next)::

      /vagrant/setup-vm.sh

    .. important:: Be sure to accept the Android SDK license prompt before continuing.

    .. code-block:: python

      cd /vagrant/examples

..

6. For **OS X/Linux**:

  a. Download ``vagrant-bundle.tar.gz`` from the `Anki SDK website <http://cozmosdk.anki.com/>`_ .
  b. Open a Terminal window.

    1. Enter the following commands::

        cd (location you downloaded the vagrant-bundle.tar.gz file)
        tar -xzf vagrant-bundle.tar.gz
        cd vagrant-bundle

    2. Enter the following command::

        vagrant up

    .. important:: Wait for `vagrant up` to completely finish before continuing to the next step.

  d. Navigate to where the Virtual Machine is currently running.
  e. Within the VM, open xterm. Once xterm is open, run the following commands in order (wait for each command to finish before entering the next)::

        /vagrant/setup-vm.sh

    .. important:: Be sure to accept the Android SDK license prompt before continuing.

    .. important:: If using an Android device, the device will will prompt with *"Allow USB Debugging?"*. Tap **OK** to allow this option.

    .. code-block:: python

        cd /vagrant/examples

7. Make sure Cozmo is powered on and charged. Connect to the Cozmo robot's WiFi from the mobile device and then connect to the Cozmo robot within the app.
8. Enter SDK mode on the app.

    a. On the Cozmo app, tap the gear icon at the top right corner to open the Settings menu.
    b. Swipe left to show the Cozmo SDK option and tap the **Enable SDK** button.

9. To run a program enter the following into the virtual machine's Terminal prompt::

        ./program_name.py

For example, to run the Hello World example program, you would type ``./hello_world.py``.
