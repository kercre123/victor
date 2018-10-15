.. _advanced-tips:

#############
Advanced Tips
#############


^^^^^^^^^^^^^^^^^^^^^
Set ANKI_ROBOT_SERIAL
^^^^^^^^^^^^^^^^^^^^^

In order to avoid entering Vector's serial number for each program run,
you can create enviroment variable ``ANKI_ROBOT_SERIAL``
and set it to Vector's serial number::

    export ANKI_ROBOT_SERIAL=00e20100


^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Set ANKI_ROBOT_HOST and VECTOR_ROBOT_NAME
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

When running ``configure.py``, you must provide Vector's ip and name.
To avoid typing these in, you can instead create environment variables
ANKI_ROBOT_HOST and VECTOR_ROBOT_NAME. Then ``configure.py`` will automatically pick
up those settings::

    export ANKI_ROBOT_HOST="192.168.42.42"
    export VECTOR_ROBOT_NAME=Vector-A1B2



----

`Terms and Conditions <https://www.anki.com/en-us/company/terms-and-conditions>`_ and `Privacy Policy <https://www.anki.com/en-us/company/privacy>`_

`Click here to return to the Anki Developer website. <http://developer.anki.com>`_
