#!/usr/bin/env python3

import sys,os,subprocess,struct
sys.path.insert(0, "../tools")
from wifiAppImageHeader import fix_image

BUILD_DIR = 'build'
FW_DIR    = 'firmware'
STAGING   = '../build'
SECTOR_SIZE = 4096
WIFI_IMAGE_SECTORS = 0xc8-0x80

env = os.environ.copy()
env['PATH'] += ":/opt/xtensa-lx106-elf/bin"
env['SDK_BASE'] = "../espressif/"
env['COMPILE']  = "gcc"

if not os.path.isdir(BUILD_DIR):
    os.mkdir(BUILD_DIR)

assert subprocess.call(['make'], env=env) == 0

img = open(os.path.join(FW_DIR, "upgrader-image.bin"), "wb")

fix_image(os.path.join(FW_DIR, "upgrader.bin"), img)
fillLen = SECTOR_SIZE - img.tell()
img.write(b"\xff" * fillLen)

img.write(open(os.path.join(STAGING, "esp.factory.bin"), "rb").read())
fillLen = ((WIFI_IMAGE_SECTORS + 1) * SECTOR_SIZE) - img.tell()
if fillLen < 0:
    sys.stderr.write("WiFi image too long!")
    sys.exit(1)
img.write(b"\xff" * fillLen)

img.write(open(os.path.join(STAGING, "factory.safe"), "rb").read())
payloadSize = img.tell() - SECTOR_SIZE

img.seek(SECTOR_SIZE - 4)
img.write(struct.pack("I", payloadSize))

img.close()
