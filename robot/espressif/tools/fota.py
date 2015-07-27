#!/usr/bin/env python
"""A python script to send over the air upgrades to the Espressif"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, socket, struct, threading, time

COMMAND_PORT = 8580

USERBIN1 = os.path.join("bin", "upgrade", "user1.1024.new.2.bin")
USERBIN2 = os.path.join("bin", "upgrade", "user2.1024.new.2.bin")
USERBIN1_ADDR = 0x01000
USERBIN2_ADDR = 0x81000

FPGA_BIN = os.path.join("..", "fpga", "fpgaisp_Implmnt", "sbt", "outputs", "bitmap", "top_bitmap.bin")

FPGA_FW_ADDR = 0x80000

USAGE = """
%s <mode> <robot IP address>

Where mode is one of:
e   Upgrade espressif firmware
f   Write FPGA firmware
""" % (sys.argv[0])

class UpgradeCommandFlags:
    NONE       = 0x00 # No special actions
    WIFI_FW    = 0x01 # Upgrade the wifi (Espressif firmware)
    CTRL_FW    = 0x02 # Upgrade the robot supervisor firmware
    FPGA_FW    = 0x04 # Upgrade the FPGA image
    BODY_FW    = 0x08 # Upgrade the body board firmware
    CONFIG     = 0x10 # Send non-volatile configuration parameters to the robot
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

class NVParams(struct.Struct):
    PREFIX  = 0xC020f3fa
    PADDING = (0xff, 0xff)
    def __init__(self, ssid, pkey, wifiOpMode, wifiChannel):
        struct.Struct.__init__(self, "<I32s64sBBBB") # See nv_params.h for the official definition
        self.ssid = ssid
        self.pkey = pkey
        self.wifiOpMode  = wifiOpMode
        self.wifiChannel = wifiChannel
    def pack(self):
        data = struct.Struct.pack(self, self.PREFIX, self.ssid, self.pkey, self.wifiOpMode, self.wifiChannel, *self.PADDING)
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

def doFPGAUpgrade(roboCon):
    "Send new FPGA firmware to the robot"
    fw = open(FPGA_BIN, 'rb').read() + bytes(range(64))
    sys.stdout.write("Sending FPGA FW\r\n")
    roboCon.send(UpgradeCommand(FPGA_FW_ADDR, len(fw), UpgradeCommandFlags.FPGA_FW).pack())
    sendFW(roboCon, fw)
    roboCon.close()
    sys.stdout.write("\r\nFinished sending FPGA firmware\r\n")

def doUpgrade(robotIp, kind, *rest):
    "Communicate with the espressif for the upgrade"
    conn = socket.socket()
    conn.connect((robotIp, COMMAND_PORT))
    if kind == UpgradeCommandFlags.WIFI_FW:
        doWifiUpgrade(conn)
    elif kind == UpgradeCommandFlags.FPGA_FW:
        doFPGAUpgrade(conn)
    elif kind == UpgradeCommandFlags.CONFIG:
        sendNVC(conn, *rest)
    else:
        raise ValueError("Upgrade type implemented yet :-(")

def sendNVC(roboCon, *rest):
    "Sends non volatile configuration parameters to the robot"
    sys.stdout.write("Sending NV config command\r\n")
    nvparams = NVParams(*rest)
    roboCon.send(UpgradeCommand(0xFFFFffff, nvparams.size, UpgradeCommandFlags.CONFIG).pack())
    ans = roboCon.recv(1500)
    if ans != b"NEXT":
        sys.exit("Unexpected return from robot: \"{}\"".format(ans))
    else:
        roboCon.send(nvparams.pack())
        roboCon.close()
        sys.stdout.write("Finished sending non-volatile configuration\r\n")

if __name__ == '__main__':
    if len(sys.argv) < 3:
        sys.stderr.write(USAGE)
        sys.exit(-1)
    mode = sys.argv[1]
    robotHostname = sys.argv[2]
    if mode == "e":
        doUpgrade(robotHostname, UpgradeCommandFlags.WIFI_FW)
    elif mode == "f":
        doUpgrade(robotHostname, UpgradeCommandFlags.FPGA_FW)
    elif mode == "c":
        if len(sys.argv) > 3:
            ssid = sys.argv[3].encode()
        else:
            sys.exit("You must specify an SSID to set")
        if len(sys.argv) > 4:
            pkey = sys.argv[4].encode()
        else:
            pkey = b"2manysecrets"
        if len(sys.argv) > 5:
            try:
                wifiOpMode = int(eval(sys.argv[5]))
            except:
                sys.exit("Couldn't interprate \"{}\" as a wifi op mode".format(sys.argv[5]))
        else:
            wifiOpMode = 0x02
        if len(sys.argv) > 6:
            try:
                wifiChannel = int(eval(sys.argv[6]))
            except:
                sys.exit("Couldn't interprate \"{}\" as a wifi channel".format(sys.argv[6]))
        else:
            wifiChannel = 2
        doUpgrade(robotHostname, UpgradeCommandFlags.CONFIG, ssid, pkey, wifiOpMode, wifiChannel)
    else:
        sys,stderr.write(USAGE)
        sys.exit(-2)
