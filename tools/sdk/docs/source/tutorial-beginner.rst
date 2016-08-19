========================================
Programming Cozmo - Beginner's Tutorials
========================================

Cozmo is programmed in Python_. If you are new to Python, `After Hours Programming <http://www.afterhoursprogramming.com/tutorial/Python/Overview/>` and `Codecademy <http://www.codecademy.com/tracks/python>` offer beginner's courses in learning Python; `Python.org's website <https://wiki.python.org/moin/BeginnersGuide/NonProgrammers>` offers a more comprehensive list of video and web tutorials. This tutorial assumes you have a minimal understanding of programming in general.

-----------------------
Example 1 - Turn Around
-----------------------

In this example, you will tell Cozmo to make a 90 degree turn in place and play a victory animation.

^^^^^^^^^^^^^^^^^^^
Writing the Program
^^^^^^^^^^^^^^^^^^^

1. In your source code editor, create a new document (*File -> New Document*).
2. First, you need to tell the program to import some important information. Type the following lines into your document exactly as shown::

import asyncio

import cozmo
from cozmo.util import degrees


  1. ``import asyncio`` is a necessary module that assists the computer and robot in communicating.
  2. ``import cozmo`` allows your program to access the information contained within the ``cozmo`` module.
  3. ``from cozmo.util import degrees`` sets this program up to use degrees as a standard of measurement.

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


  1. `anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)` triggers Cozmo to play a specific animation - in this case, his "Major Win" happy dance.
  2. ``anim.wait_for_completed`` is a signal that makes sure Cozmo completes his dance before performing his next action.

6. Type in the last three lines::

if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

  1. ``cozmo.setup_basic_logging()`` tells the program to alert you if any errors occur when running the program.
  2. ``cozmo.connect(run)`` tells the program to run as soon as Cozmo connects to the computer.

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

    cd cozmo-one/tools/sdk/tcprelay_usbmux_p

  2. Next, type the following into the same window and press Enter::

    ./openSdkTcpRelay.sh

  .. warning:: Do NOT close the first Terminal window. Closing the first Terminal window while operating with the SDK will close communications with the Cozmo robot and cause errors within the program.

4. In the *second* Terminal window, type the following and press Enter::

    cd cozmo-one/tools/sdk

  The second Terminal window is the one where you will execute programs for Cozmo to run.

5. Type the following into the second Terminal window and then press Enter::

  ./turnaround.py

3. If done correctly, Cozmo will turn and do a happy dance.
