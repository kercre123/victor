# Victor Embedded Web Server

Victor has an embedded web server that currently can be used to see files and folders, and access console variables and functions.  It will later be expanded with more features.  It can be used with a real robot and/or with Webots.

## To access from a browser

When running on a real robot, in a browser, enter the IP address of the robot and port 8887, 8888 or 8889, e.g. 192.168.40.221:8888

If you've set up your robot's IP in hosts file and named it victor you can also use victor:8888

When running webots, use 127.0.0.1:8888 (or localhost:8888)

- Port 8887: The web server that runs in its own process
- Port 8888: The web server that runs as part of the engine process, and can be used to access the engine's console vars and funcs
- Port 8889: The web server that runs as part of the anim process, and can be used to access the anim process's console vars and funcs

## Username/password

In a shipping build configuration, you will be asked for username and password, which are 'anki' and 'overdrive' respectively.

## Folders/files

Folders and files within Victor can be examined by clicking on one of the four folder buttons *persistent*, *resources*, *cache*, and *currentgamelog*. The corresponding folder contents will be shown.  Clicking on folder links will navigate to the corresponding folder.  Clicking on a file link with serve up the file (as a download, or in the case of files like .json files, right in the web page.)

## Console vars and functions

The `/consolevars` request returns a dynamically-generated web page with an interface to the console variables and console functions.

`/consolevarlist` returns a list of the registered console variables.  One can optionally specify a filter for the initial characters of the variable names,
e.g. `/consolevarlist?key=foo`

`/consolevarget?key=name_of_variable` returns the current value of the specified console variable.

`/consolevarset?key=name_of_variable&value=new_value` sets the specified console variable to the specified value.

`/consolefunclist` returns a list of the registered console functions.  One can optionally specify a filter for the initial characters of the variable names, e.g. `/consolefunclist?key=foo`

`/consolefunccall?func=name_of_function&args=arguments` calls the specified console function with the specified arguments string.  Spaces in the arguments should be represented as plus signs (+) and quote marks should be escaped (e.g.
\")

## Use from scripts or command line

You can make requests to the web server from a script or command line, e.g.
`curl victor:8888/consolevarget?key=ForceDisableABTesting`

To view the contents of the file system:

```
curl http://<ip address>:<8887 or 8888 or 8889>/persistent
curl http://<ip address>:<8887 or 8888 or 8889>/resources
curl http://<ip address>:<8887 or 8888 or 8889>/cache
curl http://<ip address>:<8887 or 8888 or 8889>/currentgamelog
```

To download a file from the file system:

`curl http://127.0.0.1:8888/resources/nameoffile.txt --output nameoffile.txt`

To upload a file:

`curl -T network.json http://127.0.0.1:8888/resources/`

To download, modify and upload a file with curl:

```
curl -o nameoffile.txt http://192.168.1.36:8888/resources/nameoffile.txt
nano nameoffile.txt
curl -T nameoffile.txt http://127.0.0.1:8888/resources/
```

## WebViz

Visit <http://victor:8888/webViz.html> for a collection of debug output and data visualization. Each tab corresponds to a javascript module that decides how to display data sent from the engine.

To create a new module, make a copy of the file [module.js.template](../../resources/webserver/webVizModules/module.js.template) 
and follow the instructions there. To send data from the engine, pass a Json::Value blob
and your target module name to WebService::SendToWebViz

## Currently not working

The 'daslog' button, and /daslog request, are currently not working.
