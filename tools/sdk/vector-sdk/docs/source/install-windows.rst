.. _install-windows:

######################
Installation - Windows
######################

This guide provides instructions on installing the Vector SDK for computers running with a Windows operating system.

^^^^^^^^^^^^^
Prerequisites
^^^^^^^^^^^^^

* Vector has been set up with the Vector companion app.
* You have successfully created an Anki account.


^^^^^^^^^^^^^^^^^^^
Python Installation
^^^^^^^^^^^^^^^^^^^

Download the `Python 3.5.1 (or later) executable file from Python.org <https://www.python.org/downloads/>`_ and
run it on your computer.

.. important:: We recommend that you tick the "Add Python 3.5 to PATH" checkbox on the Setup screen.

^^^^^^^^^^^^^^^^
SDK Installation
^^^^^^^^^^^^^^^^

To install the SDK, type the following into the Command Prompt window::

    pip3 install --user cozmo[camera]

Note that the [camera] option adds support for processing images from Vector's camera.

"""""""""""
SDK Upgrade
"""""""""""

To upgrade the SDK from a previous install, enter this command::

    pip3 install --user --upgrade cozmo

^^^^^^^^^^^^^^^
Troubleshooting
^^^^^^^^^^^^^^^

Please see the :doc:`Troubleshooting </troubleshooting>` page for tips, or visit the `Anki SDK Forums <https://forums.anki.com/>`_ to ask questions, find solutions, or for general discussion.

----

`Terms and Conditions <https://www.anki.com/en-us/company/terms-and-conditions>`_ and `Privacy Policy <https://www.anki.com/en-us/company/privacy>`_

`Click here to return to the Anki Developer website. <http://developer.anki.com>`_
