#!/usr/bin/env python
"""A python script to send over the air upgrades to the Espressif"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, socket, struct, threading, time

COMMAND_PORT = 8580

USERBIN1 = os.path.join("bin", "upgrade", "user1.1024.new.2.bin")
USERBIN2 = os.path.join("bin", "upgrade", "user2.1024.new.2.bin")
USERBIN1_ADDR = 0x01000
USERBIN2_ADDR = 0x81000

USAGE = """
%s <mode> <robot IP address>

Where mode is one of:
e   Upgrade espressif firmware
""" % (sys.argv[0])

class UpgradeCommandFlags:
    NONE       = 0x00 # No special actions
    WIFI_FW    = 0x01 # Upgrade the wifi (Espressif firmware)
    CTRL_FW    = 0x02 # Upgrade the robot supervisor firmware
    RTIP_FW    = 0x04 # Upgrade the real time image processor
    BODY_FW    = 0x08 # Upgrade the body board firmware
    ASK_WHICH  = 0x80 # Ask the espressif which firmware it wants


class UpgradeCommand(struct.Struct):
    PREFIX = b"robotsRising"
    def __init__(self, flashAddress=0xFFFFffff, length=0, flags=UpgradeCommandFlags.NONE):
        struct.Struct.__init__(self, "<12sIII")
        self.flashAddress = flashAddress
        self.length = length
        self.flags  = flags
    def pack(self):
        data = struct.Struct.pack(self, self.PREFIX, self.flashAddress, self.length, self.flags)
        print("-->", str(data))
        return data

def sendFW(roboCon, fw):
    "Send data to the robot in 1KB chunks spaced out to allow time for flash writes"
    for i in range(0, len(fw), 1024):
        cmd = roboCon.recv(1600)
        if cmd == b"next":
            roboCon.send(fw[i:i+1024])
            sys.stdout.write('.')
            sys.stdout.flush()
        else:
            sys.stderr.write("Unexpected response while sending FW: \"{}\"\r\n".format(cmd))
            break

def doWifiUpgrade(roboCon):
    roboCon.send(UpgradeCommand(flags=UpgradeCommandFlags.WIFI_FW | UpgradeCommandFlags.ASK_WHICH).pack())
    answer = roboCon.recv(1600);
    if len(answer) != 4:
        sys.stderr.write("Unexpected answer from Espressif: %s\r\n" % str(answer))
        return
    header, flags, which = struct.unpack("2sBB", answer)
    if not ((header == b"OK") and (flags == (UpgradeCommandFlags.WIFI_FW | UpgradeCommandFlags.ASK_WHICH))):
        sys.stderr.write("Unexpected answer from Espressif for wifi upgrade: {} {} {}\r\n".format(header, flags, which))
        return
    if which == 1:
        fw = open(USERBIN1, "rb").read()
        fwWriteAddr = USERBIN1_ADDR
        sys.stdout.write("Sending userbin1\r\n")
    elif which == 0:
        fw = open(USERBIN2, "rb").read()
        fwWriteAddr = USERBIN2_ADDR
        sys.stdout.write("Sending userbin2\r\n")
    else:
        sys.stderr.write("Unexpected userbin specified for Espressif WiFi upgrade: %d\r\n" % which)
        return
    roboCon.send(UpgradeCommand(fwWriteAddr, len(fw), UpgradeCommandFlags.WIFI_FW).pack())
    answer = roboCon.recv(1600)
    if (len(answer) != 4):
        sys.stderr.write("Unexpected answer from Espressif {}\r\n".format(answer))
        return
    header, flags, which = struct.unpack("2sBB", answer)
    if header != b"OK":
        sys.stderr.write("Unexpected answer from Espressif {} {} {}\r\n".format(header, flags, which))
        return
    sendFW(roboCon, fw)
    roboCon.close()
    sys.stdout.write("\r\nFinished sending WiFi firmware\r\n")

def doUpgrade(robotIp, kind):
    "Communicate with the espressif for the upgrade"
    conn = socket.socket()
    conn.connect((robotIp, COMMAND_PORT))
    if kind == UpgradeCommandFlags.WIFI_FW:
        doWifiUpgrade(conn)
    elif kind == UpgradeCommandFlags.FPGA_FW:
        doFPGAUpgrade(conn)
    else:
        raise ValueError("Upgrade type implemented yet :-(")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.stderr.write(USAGE)
        sys.exit(-1)
    mode = sys.argv[1]
    robotHostname = sys.argv[2]
    if mode == "e":
        doUpgrade(robotHostname, UpgradeCommandFlags.WIFI_FW)
    else:
        sys,stderr.write(USAGE)
        sys.exit(-2)
