-------------
What the heck is going on here?
-------------

Cozmo's RF accessories (such as cube and charger) use One-Time-Programmable ROM.  Published firmware sits on the accessory forever and can't be overwritten.  This enables a different style of programming, where you build your program as a series of patches atop the original.

BECAUSE NOT A SINGLE BIT CAN CHANGE, IT MAKES NO SENSE TO DO AUTOMATED BUILDS OF PATCHES.  You must commit exactly the version you built and tested, bit-for-bit.

All accessories ship with 'boot' installed, and can contain up to 14 additional patches.  The first patch installed is "p2" (at 0x3400), up to "p15" (at 0x0).

The compiler used is Keil C51.  The free edition is sufficient to develop a patch.  You can start your patch by copying the previous one to a new folder, and subtracting 0x400 from the "Eprom Start" in the Options/Target dialog.  Check the build output and .safe filename (in releases/) to make sure you are building the patch number you want.

If you need more than 1KB for your patch, talk to Nathan.  You can span multiple patches to get more space, or use more functions inside boot (but only by examining the boot.map file and setting aside RAM locations to cover the variables you need).

You must NEVER recompile a patch once it leaves the building - just copy that code to a new directory and start on a new patch.  If it makes it to QA with a serious error, they'll need to throw away all their accessories that were "permanently burned" with your buggy patch.

We have special "nRF24" cubes available for testing your patches - these can be programmed again and again.  Use them before committing anything to releases!

The firmware is deployed by syscon (the robot body).  Update the 'pointer' in firmware.s to your new patch and rebuild it.  It will be automatically applied to any cube that robot attaches to.
