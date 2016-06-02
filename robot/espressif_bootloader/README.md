# Espressif bootloader for COZMO
This folder contains source code for the bootloader on Cozmo's WiFi Processor

## Open Source Acknowledgement
This bootloader is based on rBoot by Richard A Burton (richardaburton@gmail.com) which is open source software under an
MIT License. The source code for rBoot may be found at https://github.com/raburton/rboot

## Theory of operation
Cozmo has slots for 3 application images

0. The factory firmware application which is never replaced and is used for factory restore.
1. Application image A
2. Application image B

At boot, the bootloader will select the appropriate image to boot as following

```
If recovery mode pin is held high:
  Select factory firmware
Else:
  Check sha1 of application image with higher image number
  If good:
    boot that image
  Else:
    Check sha1 of other application image
    If good:
      boot that image
    Else:
      boot factory image
```
