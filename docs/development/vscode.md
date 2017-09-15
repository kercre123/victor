# Visual Studio Code

Visual Studio Code (VSCode) is the main supported environment for writing code that will run on the embedded android system. Xcode is supported for debugging and profiling on Mac, but not much effort has been made to make it nice to use beyond the default cmake generated projects.

## Why VSCode?

- cross-platform
- decent support for C/C++
- supports creating/running custom script-based tasks
- working GUI front-end for remote debugging with gdb/lldb

## Download

Download and install the [latest version of VSCode](https://code.visualstudio.com/) unless otherwise stated.


## Required Extensions

### C/C++ (Microsoft)

https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools

```
ext install cpptools
```

### LLDB

https://marketplace.visualstudio.com/items?itemName=vadimcn.vscode-lldb

```
ext install vscode-lldb
```


## Recommended Extensions


### Header source

Provides fast header/source switch functionality.

https://marketplace.visualstudio.com/items?itemName=ryzngard.vscode-header-source

```
ext install vscode-header-source
```


### CMake Tools

https://marketplace.visualstudio.com/items?itemName=vector-of-bool.cmake-tools

```
ext install cmake-tools
```


### CMake

https://marketplace.visualstudio.com/items?itemName=twxs.cmake

```
ext install cmake
```
