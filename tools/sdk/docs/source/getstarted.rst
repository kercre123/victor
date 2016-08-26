=================================================
Getting Started With the Cozmo SDK - OS X / Linux
=================================================

To make sure you get the best experience possible out of the SDK, please ensure you have followed the steps in the :doc:`Installation Guide </install>`.

-------------------
Starting Up the SDK
-------------------

1. Plug the smartphone containing the Cozmo app into your computer.
2. Open the Cozmo app on the phone. Make sure Cozmo is on and connected to the app via WiFi.
3. On the computer, open two Terminal windows, and change to the directory where you extracted the sdk (e.g. cd CozmoSdk) 

.. important:: Windows users will need to install a bash shell on their systems. Windows 10 (64-bit) "has a bash shell as of the Anniversary Update <https://msdn.microsoft.com/commandline/wsl/install_guide>", or users on an earlier version of Windows may wish to use a utility like "win-bash <http://win-bash.sourceforge.net/>" or "MSYS2 <https://sourceforge.net/projects/msys2/>". Anki does not provide support for third-party programs.

  1. In the first Terminal window, type the following and press Enter::

      cd tcprelay

  2. Next, open communications between the robot and the computer.

    1. For iOS devices, type the following into the same window and press Enter::

      ./iosSdkTcpRelay.sh

    2. For Android devices, type the following into the same window and press Enter::

      ./androidSdkTcpRelay.sh

      .. important:: Make sure adb (Android Debug Bridge) is installed on your system prior to this step.

  .. warning:: Do NOT close the first Terminal window. Closing the first Terminal window while operating with the SDK will close communications with the Cozmo robot and cause errors within the program.

4. In the *second*  Terminal window, maks sure you're in the root of the sdk directory::
      
  The second Terminal window is the one where you will execute programs for Cozmo to run.

---------------------------
First Steps - "Hello World"
---------------------------

Let's test your new setup by running a very simple program. This program instructs Cozmo to say "Hello, World!" - a perfect way to introduce him and you to the world of programming.

^^^^^^^^^^^
The Program
^^^^^^^^^^^

1. Make sure the two Terminal windows are still open.
2. Type the following into the second Terminal window and then press Enter::

    ./run-example.sh examples/hello_world.py

3. If done correctly, Cozmo will say "Hello, World!"

.. warning:: If Cozmo does not perform as expected, look at the first Terminal window and make sure no error messages appeared. If you continue to have issues, please seek help in the Forums.

The code for the Hello World program is::

  import cozmo

  '''Simplest "Hello World" Cozmo example program'''

  def run(coz_conn):
    coz = coz_conn.wait_for_robot()
    coz.say_text("Hello World").wait_for_completed()

  if __name__ == '__main__':
    cozmo.setup_basic_logging()
    cozmo.connect(run)

We can edit this code to make Cozmo say something new. Let's write our first program using this code.

--------------------------
Next Steps - "Night-Night"
--------------------------

1. Open a new document in a source code editor or plain-text editor.

.. information:: Free source code editors, such as `Atom <https://atom.io>`, `Sublime <https://www.sublimetext.com>`, `TextWrangler <http://www.barebones.com/products/textwrangler/>` or `Notepad++ <http://notepad-plus-plus.org>`, can be found online. Anki does not provide tech support for third-party source code editors.

2. Copy the code from the Hello World program and paste it into the new document.
3. Each line in the program relates to a specific function. To learn more, see :ref: `tutorial-beginner`.
4. Move to line x in the program.

  1. Select the phrase "Hello World". Do NOT select the parentheses or quotation marks around the phrase; those are necessary for Python to properly parse the command.
  2. Type in the new phrase you would like Cozmo to say. In this example, Cozmo will say "Night Night"::

      coz.say_text("Night Night").wait_for_completed()

5. At the top of the screen, select *File -> Save As*, and save the program in the *examples* directory as ``nightnight.py``.
6. Now you can run your program. Open the second Terminal window, type in the following, and press Enter::

    ./run-example.sh examples/nightnight.py

7. If done correctly, Cozmo will say the new phrase.
