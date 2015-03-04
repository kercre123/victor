"""
Torpedo UDP sub server for communicating with blocks
"""
__author__ = "Daniel Casner"

from subserver import *
import sys, os, socket, time, struct
import messages

class Block(object):
    "A class for keeping track of the state of an individual block"
    def __init__(self, alias, ip, port):
        "Initalize a basic block"
        self.alias = alias
        self.address = (ip, port)

BLOCK_TABLE = {
    0: Block(0, '172.31.1.10', 5000),
    1: Block(1, '172.31.1.11', 5000),
    2: Block(2, '172.31.1.12', 5000),
    3: Block(3, '172.31.1.13', 5000),
    4: Block(4, '172.31.1.14', 5000)
}

class BlockProxyServer(BaseSubServer):
    "A proxy server for connecting to blocks"

    def __init__(self, server, verbose=False):
        "Sets up the subserver instance"
        BaseSubServer.__init__(self, server, verbose):
        self.blockSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def stop(self):
        "Tell all the blocks to shut down"
        # Tell all the blocks to turn off their lights
        lightsOut = messages.SetBlockLights()
        for b in BLOCK_TABLE.values():
            lightsOut.blockID = b.alias
            self.sendTo(b, lightsOut.serialize())
        # TODO add/replace with a full standby message

    def giveMessage(self, message):
        "Pass a message from the phone along"
        if ord(message[0]) == messages.SetBlockLights.ID:
            msg = messages.SetBlockLights(message)
            self.sentTo(BLOCK_TABLE[msg.blockID], msg.serialize())

    def sendTo(self, block, data):
        "send a message to a given block"
        self.blockSocket.sendto(data, block.address)
