#!/usr/bin/env python
"""A python script to send over the air upgrades to the Espressif"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, os, socket, struct, threading, time

COMMAND_PORT = 8580

USAGE = """%s <robot IP address>\r\n""" % (sys.argv[0])

class UpgradeCommandFlags:
    NONE       = 0x00 # No special actions
    WIFI_FW    = 0x01 # Upgrade the wifi (Espressif firmware)
    CTRL_FW    = 0x02 # Upgrade the robot supervisor firmware
    FPGA_FW    = 0x04 # Upgrade the FPGA image
    BODY_FW    = 0x08 # Upgrade the body board firmware
    ASK_WHICH  = 0x80 # Ask the espressif which firmware it wants


class UpgradeCommand(struct.Struct):
    PREFIX = b"robotsRising"
    def __init__(self, flashAddress=0xFFFFffff, length=0, flags=UpgradeCommandFlags.NONE):
        struct.Struct.__init__(self, "12sIIB")
        self.flashAddress = flashAddress
        self.length = length
        self.flags  = flags
    def pack(self):
        return struct.Struct.pack(self, self.PREFIX, self.flashAddress, self.length, self.flags)


def doWifiUpgrade(roboConn):
    roboConn.send(UpgradeCommand(flags=UpgradeCommandFlags.WIFI_FW | UpgradeCommandFlags.ASK_WHICH).pack())
    answer = roboConn.recv(1600);
    if len(answer) != 4:
        sys.stderr.write("Unexpected answer from Espressif: %s\r\n" % str(answer))
        return
    


def doUpgrade(robotIp, kind):
    "Communicate with the espressif for the upgrade"
    conn = socket.socket()
    socket.connect((robotIp, COMMAND_PORT))
    if kind == WIFI_FW:
        doWifiUpgrade(conn)
    else:
        raise ValueError("Upgrade type implemented yet :-(")

if __name__ == '__main__':
    if len(sys.argv) != 4:
        sys.stderr.write(USAGE)
        sys.exit(-1)
    try:
        serverIp = [int(o) for o in sys.argv[1].split('.')]
        assert len(serverIp) == 4
    except:
        sys.stderr.write("Couldn't parse server IP address \"%s\"\r\n" % sys.argv[1])
        sys.exit(1)
    try:
        robotIp = [int(o) for o in sys.argv[2].split('.')]
        assert len(robotIp) == 4
    except:
        sys.stderr.write("Couldn't parse robot IP address \"%s\"\r\n" % sys.argv[2])
        sys.exit(2)
    try:
        upgradeCommand = int(sys.argv[3])
        assert upgradeCommand in (0, 1, 2, 3, 4)
    except:
        sys.stderr.write("Couldn't parse robot upgrade command \"%s\"\r\n" % sys.argv[3])
        sys.exit(3)

    httpd = httpServer.HTTPServer(('%d.%d.%d.%d' % tuple(serverIp), HTTP_SERVER_PORT), UpgradeRequestHandler)
    httpdThread = threading.Thread(target=httpd.serve_forever)
    sys.stdout.write("Starting HTTP server\r\n")
    httpdThread.start()

    sys.stdout.write("Commanding upgrade\r\n")
    commandUpgrade(serverIp, robotIp, upgradeCommand)

    sys.stdout.write("^C to quit\r\n")
    try:
        time.sleep(120)
    except KeyboardInterrupt:
        pass

    httpd.server_close()
