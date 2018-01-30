# Fixture Release Checklist

A full fixture release contains several independently updatable parts:
+ fixture firmware (for the STM MCU)
+ helper head binaries
+ DUT head image files [emmcdl]

# Steps to release
+ run `release_build.sh` to rebuild helper binaries (copied to release folder)
+ manually add `fixture.safe` firmware package to the release folder (external build/release process)
+ manually add `emmcdl` folder to the release folder (external build/release process)

# Firmware
with helper head booted, run `update_firmware.bat`
requirements: `fixture.safe` and helper `dfu` bin

# Helper
with helper head booted, run `update_helper.bat`
requirements: `helper`, `helpify`, `display` bins

# Head OS Image
with helper head booted, run `update_head_image.bat`
requirements: `headprogram` bin and `emmcdl` image folder
