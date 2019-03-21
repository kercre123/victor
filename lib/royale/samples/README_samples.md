Royale sample applications
==========================

Some of these samples are only provided in the relevant platform's SDK, for example the Android
example is omitted from the other platforms' SDKs.

For developers targeting the Windows, Mac OS and Linux platforms, the examples are sorted by
language: [C++](#cpp), [C](#c) and [C#](#csharp).

For the [Android platform](#android), the example combines native C++ with Java.

For the OpenCV, OpenNI2 and ROS frameworks, there are samples that act as drivers, to use
Royale-supported cameras as inputs to those frameworks.  These are in the
[Integration and drivers](#drivers) section.

For developers with level 2 access, there are [C++](#level2cpp) examples using the additional
functionality.


C++ examples <a name="cpp"></a>
===============================

For C++, the recommended order to read these examples is to start with sampleCameraInfo, and then
sampleRetrieveData.  These also use code from the [Utility](#utility) section.

sampleCameraInfo
----------------

This C++ example shows how to create a camera and query information about the camera, and about the
use cases that the camera supports.  It doesn't capture any images.

This sample can also be used as a tool in other workflows.  It prints the list of supported modes
(use cases) which can be used with Royaleviewer's `--mode` option and as sampleRetrieveData's
command-line argument.  For UVC and Amundsen devices (e.g. pico maxx and pico monstar), it also
prints the USB firmware version.

sampleRetrieveData
------------------

This C++ example shows how to capture image data from a camera.

It's a command line application that does not depend on any GUI toolkit, therefore it only displays
textual information and low-resolution ascii-art of the captured images.

It normally uses the camera's default use case, but the name of a use case can also be given as a
command-line argument. It can also play recorded .rrf files.

sampleRecordRRF
---------------

This C++ example shows how to record rrf-files.

It's a command line application that does not depend on any GUI toolkit, therefore it only displays
textual information.

It uses the camera's default use case.
The parameters to start recording can be passed as command-line parameter:
sampleRecord.exe C:/path/to/file.rrf [numberOfFrames [framesToSkip [msToSkip]]]
where the file parameter is required!

sampleExportPLY
---------------

This C++ example shows how to replay image data from a recorded .rrf file and to do some processing
on each frame in the .rrf, in this example it exports the data to the PLY format (a format for
storing data from 3D-scanners).  Because this example only handles recordings (not live cameras), it
doesn't have the time constraints and thread-handling which make sampleRetrieveData more complicated.

It's a command line application that takes an rrf filename as a command-line argument, and outputs
PLY files in to the current working directory, one file for every frame of the recording.

sampleSlave
---------------------

This C++ sample shows how to open a slave camera, which need to be physically connected via an 
external trigger cable.

The pico monstar camera support acting as slave (the trigger cable plugs in to the non-USB external
connector on these cameras).

sampleMultiCamera
---------------------

This C++ example shows how to open and use multiple cameras simultaneously. 

It shows how to generate an application which creates, initializes, starts and stops capturing with two 
or more cameras devices. This application doesn't capture any images, instead the datalistener is
simplified and displays a basic way of data processing.


Integration and drivers <a name="drivers"></a>
==============================================

These samples are tools for using Royale-supported cameras as the data source for image processing
frameworks.  They're implemented in C++, but the data can be accessed using the tools of the
framework.

sampleOpenCV
------------

This C++ example shows how to capture image data, fill OpenCV images and display the data with HighGUI.
It was tested with OpenCV 2.4.13.

To compile this you need to point CMake to your OpenCV installation folder, and that version of
OpenCV must have been compiled with a C++ ABI version that's compatible to both Royale and Qt (there
is more documentation about this in a comment at the top of the .cpp file).

sampleOpenNI2Driver
-------------------

This C++ example shows how Royale devices can be used with the OpenNI2 API.

OpenNI2 is a third-party API for depth sensors; the SDK can be
downloaded from [the structure.io website](https://structure.io/openni).
This example implements an OpenNI2 driver which can be used to run
OpenNI2 applications (e.g. the NiViewer sample application which is
part of the OpenNI2 installation) with devices supported by Royale.

sampleROS
---------

This C++ example is delivered for Linux and OSX platforms and shows the integration of Royale
into a ROS (Robot Operating System) node. Please refer to the README.md in the sampleROS folder
for further details.

C++ examples (Level 2 access code needed) <a name="level2cpp"></a>
==================================================================

sampleProcessingParameters
--------------------------

This C++ example shows how to open the camera with a different access level
and change parameters of the processing pipeline.

sampleRawData
--------------------------

This C++ example shows how to retrieve raw and intermediate data from a camera
by using a different callbackData.


C example <a name="c"></a>
==========================

We recommend reading the C++ sampleCameraInfo and sampleRetrieveData before reading the C example.

sampleCAPI
----------

This C example shows how to query information about the camera and capture data.

It's equivalent to a combination of the functionality of the C++ sampleCameraInfo and
sampleRetrieveData, and we recommend reading both of those samples before this one.


C# examples <a name="csharp"></a>
=================================

For C#, the recommended order to read these examples is to start with sampleDotNetCamInfo, and then
sampleDotNetRetrieveData.

sampleDotNetCamInfo
------------------------

This C# example shows how to query data about the camera on Microsoft's .NET framework.

It's a command line application without a GUI part, therefore it only displays textual information.

sampleDotNetRetrieveData
------------------------

This C# example shows how to capture data on Microsoft's .NET framework.

It's a command line application without a GUI part, therefore it only displays textual information
about the captured data.


Android example <a name="android"></a>
======================================

We recommend reading the C++ sampleCameraInfo and sampleRetrieveData examples before reading the
Android example.

native Android
--------------

This JNI example (both C++ and Java) shows how to use a USB camera on Android phones that support
acting as USB hosts.

The data is received in a C++ callback, and the sample shows how to pass the received data to Java
in the NativeCamera.AmplitudeListener::onAmplitudes method.

We recommend reading the non-Android C++ sampleCameraInfo and sampleRetrieveData examples before
reading this one, as the native code is based on those examples.


Utility <a name="utility"></a>
==============================

inc/sample\_utils
-----------------

This contains the PlatformResources utility required for the C and C++ examples to use some cameras
on some platforms. Currently it's only required for UVC cameras on Windows, as the media framework
requires the application to set up the COM environment before calling the library code.
