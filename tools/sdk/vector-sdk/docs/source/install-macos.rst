.. _install-macos:

###########################
Installation - macOS / OS X
###########################

This guide provides instructions on installing the Vector SDK for computers running with a macOS operating system.

^^^^^^^^^^^^^
Prerequisites
^^^^^^^^^^^^^

* Vector has been set up with the Vector companion app.
* You have successfully created an Anki account.
* Vector is powered on.
* Vector is connected to the same network as your computer.


^^^^^^^^^^^^^^^^^^^
Python Installation
^^^^^^^^^^^^^^^^^^^

1. Install `Homebrew <http://brew.sh>`_ on your system according to the latest instructions. If you already had brew installed then update it by opening a Terminal window and typing in the following::

    brew update

2. Once Homebrew is installed and updated, type the following into the Terminal window to install the latest version of Python 3::

    brew install python3


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Vector Authentication
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

To authenticate with the robot, type the following into the Terminal window::

    cd vector_sdk
    ./configure.py

You will be prompted for your robot's name, ip address and serial number. You will also be asked for your Anki login and password.

.. note:: Running `configure.py` will automatically download the Vector robot certificate to your computer and store credentials to allow you to connect to Vector. These credentials will be stored under your home directory in folder `.anki_vector`.

.. warning:: These credentials give full access to your robot, including camera stream, audio stream and data. Do not share these credentials.


^^^^^^^^^^^^^^^^
SDK Installation
^^^^^^^^^^^^^^^^

To install the SDK, type the following into the Terminal window::

    cd vector_sdk
    pip3 install --user vector-0.4-py3-none-any.whl

Note that the [camera] option adds support for processing images from Cozmo's camera.

"""""""""""
SDK Upgrade
"""""""""""

To upgrade the SDK from a previous install, enter this command::

    cd vector_sdk
    pip3 install --user --upgrade vector-0.4-py3-none-any.whl



^^^^^^^^^^^^^^^
Troubleshooting
^^^^^^^^^^^^^^^

Please see the :doc:`Troubleshooting </troubleshooting>` page for tips, or visit the `Anki SDK Forums <https://forums.anki.com/>`_ to ask questions, find solutions, or for general discussion.

----

`Terms and Conditions <https://www.anki.com/en-us/company/terms-and-conditions>`_ and `Privacy Policy <https://www.anki.com/en-us/company/privacy>`_

`Click here to return to the Anki Developer website. <http://developer.anki.com>`_
