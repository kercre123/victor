# Victor Cube Firmware

## Flashing cube firmware onto cubes

WARNING: This procedure will flash cube firmware onto all nearby advertising cubes, potentially pissing off other developers.

The latest cube application firmware image should be available at [robot/cube_firmware/cube.dfu](/robot/cube_firmware/app/cube.dfu). To flash this firmware on nearby cubes using the [bletool](/robot/cube_firmware/bletool), do the following:

* `cd robot/cube_firmware/bletool`
* `npm install`
* ` cd ..`
* `node bletool app/cube.dfu`

## Building cube firmware with [vmake.sh](/robot/vmake.sh)

You shouldn't really have to do this, but if you do:

* `cd robot`
* `./vmake.sh cube`
* New firmware `cube.dfu` will appear in `cube_firmware/build`
