=======================================================
Programming Cozmo - Beginner's Tutorials (Windows)
=======================================================

Cozmo is programmed in Python_. If you are new to Python, `After Hours Programming <http://www.afterhoursprogramming.com/tutorial/Python/Overview/>` and `Codecademy <http://www.codecademy.com/tracks/python>` offer beginner's courses in learning Python; `Python.org's website <https://wiki.python.org/moin/BeginnersGuide/NonProgrammers>` offers a more comprehensive list of video and web tutorials. This tutorial assumes you have a minimal understanding of programming in general.

-----------------------
The Baseline - Hello World
-----------------------

Cozmo's programs are composed of *modules*. Each module calls a specific
function. Functions can either direct Cozmo to perform an action, provide
information for the system, or set parameters necessary for proper operation.
The Hello World program is a good example of the basic modules used in
Cozmo's programs.

The code for the Hello World program looks like this::

import cozmo

'''Simplest "Hello World" Cozmo example program'''

def run(coz_conn):
  coz = coz_conn.wait_for_robot()
   coz.say_text("Hello World").wait_for_completed()

if __name__ == '__main__':
   cozmo.setup_basic_logging()
   cozmo.connect(run)

The breakdown of each module in the program is as follows:

1. ``import cozmo`` allows your program to access the information contained within the ``cozmo`` module.
2. Text sandwiched between three ' marks is a comment. Comments are placed inside code to give information to the user.
3. The next two lines tell Cozmo to connect to the system, and to wait until that connection happens before performing any other actions.
4. ``coz.say_text("Hello World").wait_for_completed`` is the main module of the program.

  1. ``coz.say_text`` initializes Cozmo's speech engine.
  2. ``("Hello World")`` tells Cozmo what to say.
  3. ``wait_for_completed`` tells Cozmo to finish speaking before doing anything else.

5. The next two lines set up a logging system so that if an error occurs, the system will inform you of what specifically went wrong.
6. The final line, ``cozmo.connect(run)``, runs the program as soon as Cozmo connects to the system.

Different types of modules will be discussed in the example programs below.

--------------------------
Example 1 - Drive Straight
--------------------------

For your first program, you will tell Cozmo to drive in a straight line for three seconds. This program will give you a simple overview of the programming process, as well as some of the building blocks necessary for the programs to work.

^^^^^^^^^^^^^^^^^^^
Writing the Program
^^^^^^^^^^^^^^^^^^^

1. In your source code editor, create a new document (*File -> New Document*).
2. First, you need to tell the program to import some important information. Type the following lines into your document exactly as shown::

import asyncio

import cozmo


  1. ``import asyncio`` is a necessary module that assists the computer and Cozmo in communicating.
  2. ``import cozmo`` allows your program to access the information contained within the ``cozmo`` module.

3. Next, you need to tell the program wait for Cozmo to connect. Type the following lines into the document exactly as shown::

  def run(coz_conn):
      coz = coz_conn.wait_for_robot()

4. Now type in the following command as shown::
      coz.drive_wheels(50,50, duration=3)

  1. The ``drive_wheels`` function directly controls all aspects of Cozmo's wheel motion.
  2. ``50,50`` is the velocity of his left and right treads, respectively. Velocity is measured in millimeters per second (mm/s). In this example, Cozmo will move forward 50 millimeters per second.
  3. ``duration=3`` specifies how long Cozmo will move. Duration is measured in seconds. In this example, Cozmo will move for three seconds.

5. Type in the last three lines::

  if __name__ == '__main__':
      cozmo.setup_basic_logging()
      cozmo.connect(run)

    1. ``cozmo.setup_basic_logging()`` tells the program to alert you if any errors occur when running the program.
    2. ``cozmo.connect(run)`` tells the program to run as soon as Cozmo connects to the computer.

6. Save the file in the *examples* directory as ``drive_forward.py``.

The completed program should look like this::

import asyncio

import cozmo
from cozmo.util import degrees

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    coz.drive_wheels(50,50, duration=3)

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)


^^^^^^^^^^^^^^^^^^^
Running the Program
^^^^^^^^^^^^^^^^^^^

1. Plug the smartphone containing the Cozmo app into your computer.
2. Open the Cozmo app on the phone. Make sure Cozmo is on and connected to the app via WiFi.
3. On the computer, open two Command-line windows.

  1. In the first window, type the following and press Enter::

    cd CozmoSdk\sdk\tcprelay_usbmux_p

  2. Next, open communications between the robot and the computer.

      1. For iOS devices, type the following into the same window and press Enter::

        openSdkTcpRelay.bat

      2. For Android devices, type the following into the same window and press Enter::

        androidSdkTcpRelay.bat

        .. important:: Make sure adb (Android Debug Bridge) is installed on your system prior to this step.

  .. warning:: Do NOT close the first Terminal window. Closing the first Terminal window while operating with the SDK will close communications with the Cozmo robot and cause errors within the program.

  3. In the *second* Terminal window, type the following and press Enter::

      cd CozmoSdk\sdk

    The second Terminal window is the one where you will execute programs for Cozmo to run.

4. Type the following into the second Terminal window and then press Enter::

  run-example.bat examples\drive_forward.py

5. If done correctly, Cozmo will drive forward for three seconds and then stop.

-----------------------
Example 2 - Turn Around
-----------------------

Now that you have written your first program, you're ready to write a more complex program. In this example, you will tell Cozmo to make a 90 degree turn in place and play a victory animation.

^^^^^^^^^^^^^^^^^^^
Writing the Program
^^^^^^^^^^^^^^^^^^^

1. In your source code editor, create a new document (*File -> New Document*).
2. As in the first example, type the following lines into your document exactly as shown::

import asyncio

import cozmo
from cozmo.util import degrees

  1. ``from cozmo.util import degrees`` is a new module. This module sets the program up to use degrees as a standard of measurement.

3. Next, you need to tell the program wait for Cozmo to connect. Type the following lines into the document exactly as shown::

  def run(coz_conn):
      coz = coz_conn.wait_for_robot()

4. Now type in the following command as shown::

      coz.turn_in_place(degrees(90)).wait_for_completed()

  1. ``coz.turn_in_place`` directs Cozmo to turn in place.
  2. (degrees(90)) sets how far he turns in relation to where he is. Cozmo's initial position is assumed to be 0 degrees; he will turn 90 degrees, or directly to his right. The number of degrees goes from 0 - 360, where 0 will not move him and 360 moves him in a complete circle going clockwise. To make Cozmo move counter-clockwise, enter a negative number. For example, entering -90 makes Cozmo turn 90 degrees to the left.
  3. ``wait_for_completed()`` is a signal that makes sure Cozmo completes his turn before performing his next action.

5. Next, type in::

      anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
      anim.wait_for_completed()


  1. ``anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)`` triggers Cozmo to play a specific animation - in this case, his "Major Win" happy dance.
  2. ``anim.wait_for_completed`` is a signal that makes sure Cozmo completes his dance before performing his next action.

6. Type in the last three lines::

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

7. Save the file in the SDK directory as ``turnaround.py``.

The completed program should look like this::

import asyncio

import cozmo
from cozmo.util import degrees


def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    # Turn 90 degrees, play an animation, exit.
    coz.turn_in_place(degrees(90)).wait_for_completed()

    anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
    anim.wait_for_completed()


if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)


^^^^^^^^^^^^^^^^^^^
Running the Program
^^^^^^^^^^^^^^^^^^^

1. Plug the smartphone containing the Cozmo app into your computer.
2. Open the Cozmo app on the phone. Make sure Cozmo is on and connected to the app via WiFi.
3. On the computer, open two Terminal windows.

  1. In the first Terminal window, type the following and press Enter::

    cd CozmoSdk\sdk\tcprelay_usbmux_p

  2. Next, open communications between the robot and the computer.

      1. For iOS devices, type the following into the same window and press Enter::

        openSdkTcpRelay.bat

      2. For Android devices, type the following into the same window and press Enter::

        androidSdkTcpRelay.bat

        .. important:: Make sure adb (Android Debug Bridge) is installed on your system prior to this step.

  .. warning:: Do NOT close the first Terminal window. Closing the first Terminal window while operating with the SDK will close communications with the Cozmo robot and cause errors within the program.

4. In the *second* Terminal window, type the following and press Enter::

    cd cCozmoSdk\sdk

5. Type the following into the second Terminal window and then press Enter::

  run-example.bat examples\turnaround.py

3. If done correctly, Cozmo will turn and do a happy dance.

-----------------------
Example 3 - Cube Stack
-----------------------

As a third beginning tutorial, you can tell Cozmo to look around for his blocks, and to stack them one atop the other once he sees two of them.

^^^^^^^^^^^^^^^^^^^
Writing the Program
^^^^^^^^^^^^^^^^^^^

1. In your source code editor, create a new document (*File -> New Document*).
2. As in the first example, type the following lines into your document exactly as shown::

import asyncio

import cozmo

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

3. Now type in the following command as shown::

  cubes = coz.world.wait_until_observe_num_objects(num=2, object_type=cozmo.objects.LightCube, timeout=30)

  1. ``coz.world.wait_until_observe_num_objects`` directs Cozmo to wait until his sensors detect a specified number of objects.
  2. ``num=2`` specifies the number of objects Cozmo has to find in order to trigger the next behavior.
  3. ``object_type=cozmo.objects.LightCube`` directs Cozmo to specifically find his Cubes. He will not count other objects, such as your hands or other objects on the play area.
  4. ``timeout=30`` sets how long Cozmo will look for Cubes. Timeout is set in seconds.

4. Type in the following as shown::

  coz.pickup_object(cubes[0]).wait_for_completed()

  1. `coz.pickup_object` directs Cozmo to pick up an object. Note that currently, Cozmo can only pick up his Cubes.
  2. `(cubes[0])` specifies the Cube Cozmo needs to pick up; in this case, it is the first Cube Cozmo detected.
  3. `wait_for_completed()` is a signal that makes sure Cozmo completes his action before performing his next action.

5. Type in the following as shown::

  coz.place_on_object(cubes[1]).wait_for_completed()

  1. ``coz.place_on_object`` directs Cozmo to place the object he is holding on top of another object.
  2. ``(cubes[1])`` specifies the Cube Cozmo needs to place what he is holding onto; in this case, it is the second Cube Cozmo detected.
  3. ``wait_for_completed()`` is a signal that makes sure Cozmo completes his action before performing his next action.

6. Type in the last three lines::

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

7. Save the file in the SDK directory as ``cubestack.py``.

The completed program should look like this::

import asyncio

import cozmo

def run(coz_conn):
    coz = coz_conn.wait_for_robot()

  cubes = coz.world.wait_until_observe_num_objects(num=2, object_type=cozmo.objects.LightCube, timeout=30)

  coz.pickup_object(cubes[0]).wait_for_completed()
  coz.place_on_object(cubes[1]).wait_for_completed()

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

^^^^^^^^^^^^^^^^^^^
Running the Program
^^^^^^^^^^^^^^^^^^^

.. important:: Cozmo must have two cubes in visual range in order to perform this program. If there are not enough cubes around, or if he does not find both cubes in the thirty-second timeframe, he will not complete the program.

1. Plug the smartphone containing the Cozmo app into your computer.
2. Open the Cozmo app on the phone. Make sure Cozmo is on and connected to the app via WiFi.
3. On the computer, open two Terminal windows.

  1. In the first Terminal window, type the following and press Enter::

    cd CozmoSdk\sdk\tcprelay_usbmux_p

  2. Next, open communications between the robot and the computer.

      1. For iOS devices, type the following into the same window and press Enter::

        openSdkTcpRelay.bat

      2. For Android devices, type the following into the same window and press Enter::

        androidSdkTcpRelay.bat

        .. important:: Make sure adb (Android Debug Bridge) is installed on your system prior to this step.

  .. warning:: Do NOT close the first Terminal window. Closing the first Terminal window while operating with the SDK will close communications with the Cozmo robot and cause errors within the program.

4. In the *second* Terminal window, type the following and press Enter::

    cd CozmoSdk\sdk

5. Type the following into the second Terminal window and then press Enter::

  run-example.bat examples\cubestack.py

3. If done correctly, Cozmo will look around for 30 seconds, then pick up a cube and stack it atop another cube.
