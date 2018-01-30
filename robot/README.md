# Victor Robot Folder

This folder includes all the software related to the actual robot hardware.
Working on this software requires some hardware/robotics understanding.
Some components, including bootloaders and accessory firmware, are very low-level.

Innocent changes to certain low-level components will permanently damage the hardware and/or bankrupt the company.
Please do not make 'harmless changes' unless they are reviewed and approved by a robotics or hardware owner.
Owners are listed below.

## Low-level Components
* cube: Cube bootloader and runtime - Bryon Vandiver
* fixture: Factory programming firmware and related tools - Craig Rohe
* hal/core: Drivers for Victor's sensors/motors (hardware abstraction layer) - Adam Shelly
* robot: Realtime robotics control - Kevin Yoon
* syscon: Victor system controller (body) bootloader and runtime -Bryon Vandiver
* tools: Tools for low-level/hardware testing
