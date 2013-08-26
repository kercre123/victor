ImageKernelProcessing

Overview
==========
Stream camera to HDMI output with GaussHx2 and GaussVx2 kernels.


Hardware needed
==================
This software should run on MV153 board, connected to 121 (Omnivison sensors). 
You need also HDMI connected to a monitor to see the output.

Build
==================
To build the project, please type "make help" and follow the steps suggested there.
"make clean all"

Setup
==================

To use the application start moviDebugServer, then launch with "make run".

Expected output
==================
The video is shown on screen(640x360). 

User interaction
==================
DIP switch 4 controls camera flip mode
