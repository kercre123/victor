# Victor Embedded Web Server

Victor has an embedded web server that currently can be used to see files and
folders, and access console variables and functions.  It will later be expanded
with more features.  It can be used with a real robot and/or with Webots.

## To access from a browser

When running on a real robot, in a browser, enter the IP address of the robot
and port 8888, e.g. 192.168.40.221:8888

If you've set up your robot's IP in hosts file and named it victor you can also
use victor:8888

When running webots, use 127.0.0.1:8888 (or localhost:8888)

Note:  To access anim process console vars, use port 8889.  This is a hack that
will soon go away when the web server runs in its own process.

## Username/password

In a shipping build configuration, you will be asked for username and password,
which are 'anki' and 'overdrive' respectively.

## Folders/files

Folders and files within Victor can be examined by clicking on one of the five
folder buttons 'output', 'resources', 'cache', 'currentgamelog' and 'external'.
The corresponding folder contents will be shown.  Clicking on folder links will
navigate to the corresponding folder.  Clicking on a file link with serve up
the file (as a download, or in the case of files like .json files, right in the
web page.)

## Console vars and functions

The /consolevars request returns a dynamically-generated web page with an
interface to the console variables and console functions.

/consolevarlist returns a list of the registered console variables.  One can
optionally specify a filter for the initial characters of the variable names,
e.g. /consolevarlist?key=foo

/consolevarget?key=name_of_variable returns the current value of the specified
console variable.

/consolevarset?key=name_of_variable&value=new_value sets the specified console
varaible to the specified value.

/consolefunccall?func=name_of_function&args=arguments calls the specified
console function with the specified arguments string.  Spaces in the arguments
should be represented as plus signs (+) and quote marks should be escaped (e.g.
\")

## Use from scripts or command line

You can make requests to the web server from a script or command line, e.g.
curl victor:8888/consolevarget?key=ForceDisableABTesting

## Currently not working

The 'daslog' button, and /daslog request, are currently not working.
