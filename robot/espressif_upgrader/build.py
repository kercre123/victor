#!/usr/bin/env python3
"""Assembles an upgrader image"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, subprocess, struct

SECTOR_SIZE = 0x1000

def loadAndPadFirmware(fpn, size=SECTOR_SIZE):
    fw = open(fpn, 'rb').read()
    assert len(fw) <= size
    fw = fw + (b"\xff" * (size - len(fw)))
    assert len(fw) == size, len(fw)
    return fw

def makeImage():
    of = open('build/upgrader-unified-image.bin', 'wb')
    of.write(loadAndPadFirmware('firmware/upgrader.bin', SECTOR_SIZE-4)) # -4 for the four bytes of padding in OTA firmware
    of.write(loadAndPadFirmware('../espressif_bootloader/firmware/cboot.bin'))
    factoryFirmware  = loadAndPadFirmware("../staging/esp.factory.bin", 0x45000)
    recoveryFirmware = loadAndPadFirmware("../staging/factory.safe",    0x25000)
    of.write(struct.pack('I', len(factoryFirmware) + len(recoveryFirmware)))
    of.write(factoryFirmware)
    of.write(recoveryFirmware)
    of.close()

def build():
    e = os.environ.copy()
    e['PATH'] = "/opt/xtensa-lx106-elf/bin:" + e['PATH']
    if not os.path.isdir('build'): os.mkdir('build')
    if not os.path.isdir('firmware'): os.mkdir('firmware')
    subprocess.call(['make', 'SDK_BASE=../espressif/', 'COMPILE=gcc'], shell=False, env=e)
    makeImage()
    subprocess.call(['python3', 'tools/sign.py', '--wifi', 'espressif_upgrader/build/upgrader-unified-image.bin', 'staging/upgrader.safe'], cwd='..', env=e)

if __name__ == '__main__':
    build()
