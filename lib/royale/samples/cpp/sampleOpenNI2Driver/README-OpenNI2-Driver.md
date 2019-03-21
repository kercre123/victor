OpenNI2 Driver for Royale ToF cameras
=====================================

This driver was developed for OpenNI2 version 2.2 on the Linux 64 bit
and Windows x86 platforms supported by Royale.
The current implementation delivers a depth stream only. Switching
between usecases is already supported, but due to limitations in the
API it is not possible to distinguish between usecases having the same
frame rate (these will show up as duplicate entries in the list of
video modes). Note that some of the sample programs packaged with
OpenNI2 also require a color video stream; these samples will not work
with this driver. The NiViewer2 tool also tries opening a color and an
IR stream in addition to the depth stream, but will also work without
(with limited functionality).


Using the OpenNI2 driver on Windows
-----------------------------------

On Windows, OpenNI2 expects the driver repository in the folder
OpenNI2/Drivers (relative to the location of the application binary)
by default. This can be overriden by setting the Repository in section
Drivers in the OpenNI.ini configuration file (which is expected to be
in the same folder as the application binary).
The following files should be placed in the driver repository:

- royaleONI2.dll
- royale.dll
- spectre3.dll

Known issues:
- UVC based devices require the application to properly initializes
  the Windows COM subsystem. The sample programs and the NiViewer tool
  delivered with the OpenNI2 distribution don't do this, so UVC based
  devices currently don't work (and are not detected) with these.


Using the OpenNI2 driver on Linux
---------------------------------

On Linux, OpenNI2 uses a system-wide configuration. In the Ubuntu
package, the driver repository is in /usr/lib/OpenNI2/Drivers (which
can be changed in the configuration file /etc/openni2/OpenNI.ini for
all users).

The following files should be placed in the driver repository:

- libroyaleONI2.so.0
- libroyale.so.3.7.0
- libspectre3.so
