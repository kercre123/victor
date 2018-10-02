.. _install-linux:

####################
Installation - Linux
####################

^^^^^^^^^^^^^
Prerequisites
^^^^^^^^^^^^^

* Vector has been set up with the Vector companion app.
* You have successfully created an Anki account.
* Vector is powered on.
* Vector is connected to the same network as your computer.


This guide provides instructions on installing the Vector SDK for computers running with an Ubuntu Linux operating system.

.. warning:: The Vector SDK is tested and and supported on Ubuntu 16.04 and 18.04. Anki makes no guarantee the Vector SDK will work on other versions of Linux.  If you wish to try the Vector SDK on versions of Linux *other than* Ubuntu 14.04 or 16.04, please ensure the following dependencies are installed:

  * Python 3.5.1 or later
  * pip for Python 3 (Python package installer)



^^^^^^^^^^^^^^^^^^^^^^
Ubuntu 16.04 and 18.04
^^^^^^^^^^^^^^^^^^^^^^

"""""""""""""""""""
Python Installation
"""""""""""""""""""

1. Type the following into your Terminal window to install Python::

    sudo apt-get update
    sudo apt-get install python3

2. Then install pip by typing in the following into the Terminal window::

    sudo apt install python3-pip

3. Last, install Tkinter::

    sudo apt-get install python3-pil.imagetk

""""""""""""""""
SDK Installation
""""""""""""""""

To install the SDK, type the following into the Terminal window::

    pip3 install --user vector-0.4-py3-none-any.whl

Note that the [camera] option adds support for processing images from Vector's camera.

"""""""""""
SDK Upgrade
"""""""""""

To upgrade the SDK from a previous install, enter this command::

    pip3 install --user --upgrade vector-0.4-py3-none-any.whl

^^^^^^^^^^^^^^^
Troubleshooting
^^^^^^^^^^^^^^^

Please see the :doc:`Troubleshooting </troubleshooting>` page for tips, or visit the `Anki SDK Forums <https://forums.anki.com/>`_ to ask questions, find solutions, or for general discussion.

----

`Terms and Conditions <https://www.anki.com/en-us/company/terms-and-conditions>`_ and `Privacy Policy <https://www.anki.com/en-us/company/privacy>`_

`Click here to return to the Anki Developer website. <http://developer.anki.com>`_
