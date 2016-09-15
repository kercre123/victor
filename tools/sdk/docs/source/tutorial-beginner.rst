========================================
Programming Cozmo - Beginner's Tutorials
========================================

.. warning:: THIS IS THE ALPHA BUILD OF THE COZMO SDK. All information within this SDK is subject to change without notice.

Cozmo is programmed in Python. If you are new to Python, `After Hours Programming <http://www.afterhoursprogramming.com/tutorial/Python/Overview/>`_ and `Codecademy <http://www.codecademy.com/tracks/python>`_ offer beginner's courses in learning Python; `Python.org's website <https://wiki.python.org/moin/BeginnersGuide/NonProgrammers>`_ offers a more comprehensive list of video and web tutorials. This tutorial assumes you have a minimal understanding of programming in general.

--------------------------
The Baseline - Hello World
--------------------------

Cozmo's programs are composed of *modules*. Each module calls a specific function. Functions can either direct Cozmo to perform an action, provide information for the system, or set parameters necessary for proper operation. The Hello World program is a good example of the basic modules used in Cozmo's programs.

The code for the Hello World program looks like this.

.. code-block:: python
  :linenos:

  import cozmo

  '''Simplest "Hello World" Cozmo example program
  '''

  def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    coz.say_text("Hello World").wait_for_completed()

  if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

..

The breakdown of each module in the program is as follows:

1. ``import cozmo`` allows your program to access the information contained within the ``cozmo`` module.
2. Text sandwiched between three ' marks is a comment. Comments are placed inside code to give information to the user.
3. The first line of the run function tells Cozmo to connect to the system, and to wait until that connection happens before performing any other actions.
4. ``coz.say_text("Hello World").wait_for_completed`` is the main module of the program.

  a. ``coz.say_text`` initializes Cozmo's speech engine.
  b. ``("Hello World")`` tells Cozmo what to say.
  c. ``wait_for_completed`` tells Cozmo to finish speaking before doing anything else.

5. The next two lines set up a logging system so that if an error occurs, the system will inform you of what specifically went wrong.
6. The final line, ``cozmo.connect(run)``, runs the program as soon as Cozmo connects to the system.

Different types of modules will be discussed in the example programs below.

--------------------
Running the Programs
--------------------

Example programs are available in the *examples* directory should you wish to run programs without writing them. Any programs you write, whether the examples shown here or new programs, should be saved to a new directory of your choice.

.. important:: Do NOT save example programs to the *examples* directory. Doing so will overwrite the original baseline programs.

..

To run a program for Cozmo:

.. important:: iTunes must be installed on any computer system paired with an iPhone before running any programs.

1. Plug the smartphone containing the Cozmo app into your computer.
2. Open the Cozmo app on the phone. Make sure Cozmo is on and connected to the app via WiFi.
3. On the computer, open two Terminal (iOS/Linux) / Command-line (Windows) windows.

  a. In the first window, type ``cd SDKDirectory`` where *SDKDirectory* is the name of the directory you extracted the SDK into and press Enter.

  b. Next, type the following and press Enter::

      cd tcprelay

  c. Now open communications between the robot, the phone, and the computer.

    i. For iOS/Linux systems:

    .. warning:: If switching to an iPhone from Android after running the SDK, type in `./androidSdkRemoveTcpRelay.sh` before proceeding.

      a. For iPhone, type the following into the same window and press Enter::

          ./iosSdkTcpRelay.sh

      b. For Android devices, type the following into the same window and press Enter::

          ./androidSdkAddTcpRelay.sh

    ii. For Windows systems:

    .. warning:: If switching to an iPhone from Android after running the SDK, type in `./androidSdkRemoveTcpRelay.bat` before proceeding.

      a. For iPhone, type the following into the same window and press Enter::

          ./iosSdkTcpRelay.bat

      .. important:: Communications between iPhone and a Windows system require `Python 2 <https://www.python.org/downloads/>`_ to be installed. Installing Python 2 will not uninstall Python 3 from the system.

      b. For Android devices, type the following into the same window and press Enter::

          ./androidSdkAddTcpRelay.bat

  .. important:: Make sure adb (Android Debug Bridge) is installed on your system prior to this step. See `the Beginner's Tutorial </tutorial-beginner>`_ for instructions.

  .. warning:: Do NOT close the first Terminal window. Closing the first Terminal window while operating with the SDK will close communications with the Cozmo robot and cause errors within the program.

4. In the *second* Terminal window, type ``cd SDKDirectory`` where SDKDirectory is the name of the directory you extracted the SDK into and press Enter.

5. Now you can run your program.

  a. For iOS and Linux systems, type the following into the same window and press **Enter**::

      ./run-example.sh directoryname/program_name.py

  b. For Windows systems, type the following into the same window and press **Enter**::

      run-example.bat directoryname/program_name.py

6. If done correctly, Cozmo will execute the program.

--------------------------
Example 1 - Drive Straight
--------------------------

For your first program, you will tell Cozmo to drive in a straight line for three seconds. This program will give you a simple overview of the programming process, as well as some of the building blocks necessary for the programs to work.

1. In your source code editor, create a new document (*File -> New Document*).
2. First, you need to tell the program to import some important information. Type the following lines into your document exactly as shown:

.. code-block:: python
  :linenos:

  import asyncio

  import cozmo

..

  a. ``import asyncio`` is a necessary module that assists the computer and Cozmo in communicating.
  b. ``import cozmo`` allows your program to access the information contained within the ``cozmo`` module.

3. Next, you need to tell the program wait for Cozmo to connect. Type the following lines into the document exactly as shown:

.. code-block:: python
  :lineno-start: 5

  def run(coz_conn):
      coz = coz_conn.wait_for_robot()

4. Now type in the following command as shown:

.. code-block:: python
  :lineno-start: 8

      coz.drive_wheels(50,50, duration=3)

..

  a. The ``drive_wheels`` function directly controls all aspects of Cozmo's wheel motion.
  b. ``50,50`` is the velocity of his left and right treads, respectively. Velocity is measured in millimeters per second (mm/s). In this example, Cozmo will move forward 50 millimeters per second.
  c. ``duration=3`` specifies how long Cozmo will move. Duration is measured in seconds. In this example, Cozmo will move for three seconds.

5. Type in the last three lines:

.. code-block:: python
  :lineno-start: 10

  if __name__ == '__main__':
      cozmo.setup_basic_logging()
      cozmo.connect(run)

..

    a. ``cozmo.setup_basic_logging()`` tells the program to alert you if any errors occur when running the program.
    b. ``cozmo.connect(run)`` tells the program to run as soon as Cozmo connects to the computer.

6. Save the file in the directory of your choice as ``drive_forward.py``.

The completed program should look like this.

.. code-block:: python
  :linenos:

  import asyncio

  import cozmo

  def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    coz.drive_wheels(50,50, duration=3)

  if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)


-----------------------
Example 2 - Turn Around
-----------------------

Now that you have written your first program, you're ready to write a more complex program. In this example, you will tell Cozmo to make a 90 degree turn in place and play a victory animation.

1. In your source code editor, create a new document (*File -> New Document*).
2. As in the first example, type the following lines into your document exactly as shown:

.. code-block:: python
  :linenos:

  import asyncio

  import cozmo
  from cozmo.util import degrees

..

  a. ``from cozmo.util import degrees`` is a new module. This module sets the program up to use degrees as a standard of measurement.

3. Next, you need to tell the program wait for Cozmo to connect. Type the following lines into the document exactly as shown:

.. code-block:: python
  :lineno-start: 7

  def run(coz_conn):
      coz = coz_conn.wait_for_robot()

4. Now type in the following command as shown:

.. code-block:: python
  :lineno-start: 10

      coz.turn_in_place(degrees(90)).wait_for_completed()

..

  a. ``coz.turn_in_place`` directs Cozmo to turn in place.
  b. ``(degrees(90))`` sets how far he turns in relation to where he is. Cozmo's initial position is assumed to be 0 degrees; he will turn 90 degrees, or directly to his right. The number of degrees goes from 0 - 360, where 0 will not move him and 360 moves him in a complete circle going clockwise. To make Cozmo move counter-clockwise, enter a negative number. For example, entering -90 makes Cozmo turn 90 degrees to the left.
  c. ``wait_for_completed()`` is a signal that makes sure Cozmo completes his turn before performing his next action.

5. Next, type in:

.. code-block:: python
  :lineno-start: 12

      anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
      anim.wait_for_completed()

..

  a. ``anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)`` triggers Cozmo to play a specific animation - in this case, his "Major Win" happy dance.
  b. ``anim.wait_for_completed`` is a signal that makes sure Cozmo completes his dance before performing his next action.

6. Type in the last three lines:

.. code-block:: python
  :lineno-start: 16

  if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

7. Save the file in the SDK directory as ``turnaround.py``.

The completed program should look like this.

.. code-block:: python
  :linenos:

  import asyncio

  import cozmo
  from cozmo.util import degrees


  def run(coz_conn):
    coz = coz_conn.wait_for_robot()

    coz.turn_in_place(degrees(90)).wait_for_completed()

    anim = coz.play_anim_trigger(cozmo.anim.Triggers.MajorWin)
    anim.wait_for_completed()


  if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

-----------------------
Example 3 - Cube Stack
-----------------------

As a third beginning tutorial, you can tell Cozmo to look around for his blocks, and to stack them one atop the other once he sees two of them.


1. In your source code editor, create a new document (*File -> New Document*).
2. As in the first example, type the following lines into your document exactly as shown:

.. code-block:: python
  :linenos:

  import asyncio

  import cozmo

  def run(coz_conn):
    coz = coz_conn.wait_for_robot()

3. Now type in the following command as shown::

.. code-block:: python
  :lineno-start: 8

    lookaround = coz.start_behavior(cozmo.behavior.BehaviorTypes.LookAround)

    cubes = coz.world.wait_until_observe_num_objects(num=2, object_type=cozmo.objects.LightCube, timeout=30)

..

  1. ``coz.start_behavior(cozmo.behavior.BehaviorTypes.LookAround)``

    a. ``coz.start_behavior`` initiates a specific behavior.
    b. ``cozmo.behavior.BehaviorTypes.LookAround`` is a special behavior where Cozmo will actively move around and search for objects.

  2. ``coz.world.wait_until_observe_num_objects`` directs Cozmo to wait until his sensors detect a specified number of objects.
  3. ``num=2`` specifies the number of objects Cozmo has to find in order to trigger the next behavior.
  4. ``object_type=cozmo.objects.LightCube`` directs Cozmo to specifically find his Cubes. He will not count other objects, such as your hands or other objects on the play area.
  5. ``timeout=30`` sets how long Cozmo will look for Cubes. Timeout is set in seconds.

4. Type in the following as shown::

.. code-block:: python
  :lineno-start: 10

  coz.pickup_object(cubes[0]).wait_for_completed()

..

  a. ``coz.pickup_object`` directs Cozmo to pick up an object. Note that currently, Cozmo can only pick up his Cubes.
  b. ``(cubes[0])`` specifies the Cube Cozmo needs to pick up; in this case, it is the first Cube Cozmo detected.
  c. ``wait_for_completed()`` is a signal that makes sure Cozmo completes his action before performing his next action.

5. Type in the following as shown::

.. code-block:: python
  :lineno-start: 11

    coz.place_on_object(cubes[1]).wait_for_completed()

..

  1. ``coz.place_on_object`` directs Cozmo to place the object he is holding on top of another object.
  2. ``(cubes[1])`` specifies the Cube Cozmo needs to place what he is holding onto; in this case, it is the second Cube Cozmo detected.
  3. ``wait_for_completed()`` is a signal that makes sure Cozmo completes his action before performing his next action.

6. Type in the last three lines::

.. code-block:: python
  :lineno-start: 13

  if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

7. Save the file in the SDK directory as ``cubestack.py``.

The completed program should look like this.

.. code-block:: python
  :linenos:

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

..

---------------------------
Example 4 - Sing the Scales
---------------------------

Building further on previously introduced code, let's combine your new knowledge on movement with the knowledge gained with the "Hello World" program to make Cozmo sing the scales.

^^^^^^^^^^^^^^^^^^^
Writing the Program
^^^^^^^^^^^^^^^^^^^

1. In your source code editor, create a new document (*File -> New Document*).
2. The code for the program is listed below.

  .. code-block:: python
    :linenos:

      import cozmo
      from cozmo.util import degrees

      '''Slight extension from hello_world.py - introduces for loops to make Cozmo "sing" the scales
      '''

      def run(coz_conn):
        coz = coz_conn.wait_for_robot()

        scales = ["Doe", "Ray", "Mi", "Fa", "So", "La", "Ti", "Doe"]

        # Find voice_pitch_delta value that will range the pitch from -1 to 1 over all of the scales
        voice_pitch = -1.0
        voice_pitch_delta = 2.0 / (len(scales) -1)

        # move head and lift down to the bottom, and wait until that's achieved
        coz.move_head(-5) # start moving head down so it mostly happens in parallel with lift
        coz.set_lift_height(0.0).wait_for_completed()
        coz.set_head_angle(degrees(-25.0)).wait_for_completed()

        # start slowly raising lift and head
        coz.move_lift(0.15)
        coz.move_head(0.15)

        # "sing" each note of the scale at increasingly high pitch
        for note in scales:
          coz.say_text(note, voice_pitch=voice_pitch, duration_scalar=0.3).wait_for_completed()
          voice_pitch += voice_pitch_delta

      if __name__ == '__main__':
        cozmo.setup_basic_logging()
        cozmo.connect(run)

..

The new code elements introduced in this section are as follows:
  1. ``voice_pitch`` and ``voice_pitch_delta``

      a. ``voice_pitch`` adjusts the pitch of Cozmo's voice. The valid range for pitch is on a scale from -1.0 to 1.0.
      b. ``voice_pitch_delta`` defines the valid range of Cozmo's voice pitch. The value of 2.0 is what sets the range to be from -1.0 to 1.0. Values in this line of code should not be changed.

  2. ``coz.move_head(-5)`` moves Cozmo's head down.
  3. ``coz.set_lift_height(0.0)`` sets the position of his lift. (0.0) is the lift's neutral starting position.
  4. ``coz.set_head_angle`` sets the angle Cozmo should tilt his head. In this example, it is set to degrees. (-25.0) sets the specific angle to 25 degrees tilted down.
  5. The values of ``coz.move_lift`` and ``coz.move_head`` are then set the same so that the head and lift will move in unison.
  6. The variables within the ``coz.say_text`` directive denotes what Cozmo will say, the pitch he will speak at, and the relative time period over which Cozmo will sing.
  7. ``voice_pitch += voice_pitch_delta`` serts it us so that Cozmo's voice will rise in pitch with each word he says.
