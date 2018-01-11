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

You will need to configure cpptools as described here:
https://code.visualstudio.com/docs/languages/cpp

You will need to create a c_cpp_properties.json file to locate C/C++ header files.
Bootstrap your include path by pasting something like this:
```json
{
    "configurations": [
        {
            "name": "Mac",
            "includePath": [
                "${workspaceRoot}",
                "${workspaceRoot}/robot/include",
                "${workspaceRoot}/robot/hal/include",
                "${workspaceRoot}/robot/hal/sim/include",
                "${workspaceRoot}/lib/util",
                "${workspaceRoot}/lib/util/source/anki",
                "${workspaceRoot}/lib/util/source/3rd/jsoncpp",
                "${workspaceRoot}/generated/clad/engine",
                "${workspaceRoot}/generated/clad/robot",
                "${workspaceRoot}/generated/clad/util",
                "${workspaceRoot}/coretech/generated/clad/common",
                "${workspaceRoot}/coretech/generated/clad/vision",
                "${workspaceRoot}/tools/message-buffers/support/cpp/include",
                "${workspaceRoot}/EXTERNALS/coretech_external/opencv-3.1.0/modules/core/include",
                "${workspaceRoot}/EXTERNALS/coretech_external/opencv-3.1.0/modules/highgui/include",
                "${workspaceRoot}/EXTERNALS/coretech_external/opencv-3.1.0/modules/imgcodecs/include",
                "${workspaceRoot}/EXTERNALS/coretech_external/opencv-3.1.0/modules/videoio/include",
                "${workspaceRoot}/EXTERNALS/coretech_external/opencv-3.1.0/modules/imgproc/include",
                "/Applications/Webots.app/include/controller/cpp",
                "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include/c++/v1",
                "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/8.1.0/include",
                "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/include",
                "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include",
                "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/4.2.1",
                "/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include/c++/4.2.1/tr1",
                "/usr/local/include"
                
            ],
        }
    ]
}
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
