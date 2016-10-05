==================================
Getting Started With the Cozmo SDK
==================================

.. warning:: THIS IS THE ALPHA BUILD OF THE COZMO SDK. All information within this SDK is subject to change without notice.

To make sure you get the best experience possible out of the SDK, please ensure you have followed the steps in the :doc:`Installation Guide </install>`.

-------------------
Starting Up the SDK
-------------------

1. Plug the mobile device containing the Cozmo app into your computer.
2. Open the Cozmo app on the phone. Make sure Cozmo is on and connected to the app via WiFi.
3. Swipe right on the gear icon at the top right corner to open the Settings menu.
4. Swipe left to show the Cozmo SDK option and tap the Enable SDK button.
5. On the computer, open a Terminal (iOS/Linux) / Command-line (Windows) window. Type ``cd SDKDirectory`` where *SDKDirectory* is the name of the directory you extracted the SDK into and press Enter.
6. Now you can run your program.

---------------------------
First Steps - "Hello World"
---------------------------

Let's test your new setup by running a very simple program. This program instructs Cozmo to say "Hello, World!" - a perfect way to introduce him and you to the world of programming.

^^^^^^^^^^^
The Program
^^^^^^^^^^^

1. To run the program, using the same Terminal (iOS/Linux) / Command-line (Windows) window mentioned above:

    a. For iOS and Linux systems, type the following and press **Enter**::

        python3 hello_world.py

    b. For Windows systems, type the following and press **Enter**::

        py hello_world.py

2. If done correctly, Cozmo will say "Hello, World!"

.. warning:: If Cozmo does not perform as expected, look at the first Terminal window and make sure no error messages appeared. If you continue to have issues, please seek help in the Forums.

The code for the Hello World program is:

.. code-block:: python
  :linenos:

  import sys

  import cozmo

  '''Hello World

  Make Cozmo say 'Hello World' in this simple Cozmo SDK example program.
  '''

  def run(sdk_conn):
    coz = sdk_conn.wait_for_robot()
    robot.say_text("Hello World").wait_for_completed()

  if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

    try:
       cozmo.connect(run)
    except cozmo.ConnectionError as e:
       sys.exit("A connection error occurred: %s" % e)

We can edit this code to make Cozmo say something new. Let's write our first program using this code.

--------------------------
Next Steps - "Night-Night"
--------------------------

1. Open a new document in a source code editor or plain-text editor. Free source code editors, such as `Atom <https://atom.io>`_ , `Sublime <https://www.sublimetext.com>`_ , or `TextWrangler <http://www.barebones.com/products/textwrangler/>`_ can be found online. Anki does not provide tech support for third-party source code editors.

2. Copy the code from the Hello World program and paste it into the new document.
3. Each line in the program relates to a specific function. To learn more, see :doc:`the Beginner's Tutorial </tutorial-beginner>`.
4. Move to line 15 in the program.

  1. Select the phrase "Hello World". Do NOT select the parentheses or quotation marks around the phrase; those are necessary for Python to properly parse the command.
  2. Type in the new phrase you would like Cozmo to say. In this example, Cozmo will say "Night Night"::

      robot.say_text("Night Night").wait_for_completed()

5. At the top of the screen, select *File -> Save As*, and save the program in the SDK directory as ``nightnight.py``.
6. Now you can run your program:

        a. For iOS and Linux systems, type the following into the same window and press **Enter**::

            python3 nightnight.py

        b. For Windows systems, type the following into the same window and press **Enter**::

            py nightnight.py

7. If done correctly, Cozmo will say the new phrase.
