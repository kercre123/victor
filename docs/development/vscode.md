# Visual Studio Code

Visual Studio Code (VSCode) is the main supported environment for writing code 
that will run on the Vicos system. Xcode is supported for debugging 
and profiling on Mac, but not much effort has been made to make it nice to use 
beyond the default cmake generated projects.

## Why VSCode?

- cross-platform
- decent support for C/C++
- supports creating/running custom script-based tasks
- working GUI front-end for remote debugging with gdb/lldb

## Download

Download and install the [latest version of VSCode](https://code.visualstudio.com/) 
unless otherwise stated.

## Setup

Bootstrap your VSCode configuration by copying some [template files](/templates/vscode):
```
  cd [repo]
  mkdir -p .vscode
  cp templates/vscode/*.json .vscode
```
This will give you a basic set of [task definitions](/templates/vscode/tasks.json) 
and [project settings](/templates/vscode/settings.json) so you 
don't have to start with a blank slate.

## Required Extensions

You must install a number of support extensions before you can build Anki software.
You can install extensions by typing shorthand commands into the "Go To File"
command prompt, or by opening the Extensions pane (Shift-Command-X) and using the
GUI.

Extensions in the marketplace may have very similar names.  Check the author name
to make sure you are getting the right extension. Accept no substitutes!

### C/C++ (Microsoft)

https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools

```
Command-P (Go To File)
ext install ms-vscode.cpptools
```

You will need to configure cpptools as described here:
https://code.visualstudio.com/docs/languages/cpp

You will need to create a c_cpp_properties.json file to locate C/C++ header 
files.  

You can copy [template settings](/templates/vscode/c_cpp_properties.json) 
to get started.


### LLDB

https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb

```
Command-P (Go To File)
ext install vadimcn.vscode-lldb
```


## Recommended Extensions


### Header source

Provides fast header/source switch functionality.

https://marketplace.visualstudio.com/items?itemName=ryzngard.vscode-header-source

```
Command-P (Go To File)
ext install ryzngard.vscode-header-source
```


### CMake Tools

https://marketplace.visualstudio.com/items?itemName=vector-of-bool.cmake-tools

```
Command-P (Go To File)
ext install vector-of-bool.cmake-tools
```


### CMake

https://marketplace.visualstudio.com/items?itemName=twxs.cmake

```
Command-P (Go To File)
ext install twxs.cmake
```

## Open Workspace

Start by using "File -> Open" to open your top-level victor repo.

Use "File -> Save Workspace As" to save your configuration into a VS Code 
workspace for future use.

Once you have created a workspace file, you can use "File -> Open Workspace" 
to pick up where you left off.

## Run Task: Build 

Use "Tasks -> Run Build Task" to run the default build task.  

You can set additional build flags (e.g. "-p mac" vs "-p vicos") by editing 
the task configuration, or you can create a second set of tasks and switch 
between them.

## Run Task: Deploy

After building, use "Tasks -> Run Task -> deploy:debug" to deploy executables
to the robot.

## Run Task: Debug

After deploying, use "Tasks -> Run Task -> debug:debug" to launch the debugger
on a robot process.

## Customize

VS Code is extensible and highly customizable.  Most of the configuration 
settings use a simple JSON syntax and can be changed with 
"Tasks -> Configure Tasks" or "Code -> Preferences -> Settings".

If you don't like the default settings, change them!

