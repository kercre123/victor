"""
Torpedo UDP sub server for communicating with blocks
"""
__author__ = "Daniel Casner"

from subserver import *
import sys, os, socket, select, time, struct
import messages


MTU = 1500

class Block(object):
    "A class for keeping track of the state of an individual block"
    def __init__(self, alias, ip, port):
        "Initalize a basic block"
        self.alias = alias
        self.address = (ip, port)

    def __repr__(self):
        return "Block(%d, %s, %d)" % (self.alias, self.address[0], self.address[1])

BLOCK_PORT = 6002

BLOCK_TABLE = {
    0: Block(0, '172.31.1.10', BLOCK_PORT),
    1: Block(1, '172.31.1.11', BLOCK_PORT),
    2: Block(2, '172.31.1.12', BLOCK_PORT),
    3: Block(3, '172.31.1.13', BLOCK_PORT),
    4: Block(4, '172.31.1.14', BLOCK_PORT)
}

class BlockProxyServer(BaseSubServer):
    "A proxy server for connecting to blocks"

    BLOCK_SOCK_HOST = "172.31.1.1"
    BLOCK_SOCK_PORT = 6001
    SERIAL_HEADER = "\xbe\xef"

    def __init__(self, server, verbose=False):
        "Sets up the subserver instance"
        BaseSubServer.__init__(self, server, verbose)
        self.blockSocket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.blockSocket.bind((self.BLOCK_SOCK_HOST, self.BLOCK_SOCK_PORT))

    def stop(self):
        sys.stdout.write("Closing BlockProxyServer\n")
        BaseSubServer.stop(self)
        self.standby()

    def standby(self):
        "Tell all the blocks to shut down"
        # Tell all the blocks to turn off their lights
        lightsOut = messages.SetBlockLights(blockID=255)
        # TODO add/replace with a full standby message
        self.sendBroadcast(lightsOut.serialize())

    def giveMessage(self, message):
        "Pass a message from the phone along"
        msgID = ord(message[0])
        if self.v:
            sys.stdout.write("BP UDP in: %d[%d]\n" % (msgID, len(message)))
        if msgID == messages.SetBlockLights.ID:
            msg = messages.SetBlockLights(message)
            self.sentTo(BLOCK_TABLE[msg.blockID], msg.serialize())
        elif msgID == messages.FlashBlockIDs.ID:
            self.sendBroadcast(message)

    def sendTo(self, block, message):
        "send a message to a given block"
        if self.v:
            sys.stdout.write("BP sendTo %d, %d[%d]\n" % (block.alias, ord(message[0]), len(message)))
        data = self.SERIAL_HEADER + struct.pack('I', len(message)) + message
        self.blockSocket.sendto(data, block.address)

    def sendBroadcast(self, message):
        "Send a message to all blocks"
        if self.v:
            sys.stdout.write("BP broadcast...\n")
        # UDP broadcast doesn't seem to work so iterate over all blocks instead :-/
        for b in BLOCK_TABLE.values():
            self.sendTo(b, message)

    def step(self):
        "Main loop execution step for block proxy"
        rfds, wfds, efds = select.select([self.blockSocket], [], [self.blockSocket], 1.0)
        if self.blockSocket in efds:
            if self._continue == False:
                return
            else:
                sys.stderr.write("BlockProxyServer.blockSocket in exceptional continution\n")
        if self.blockSocket not in rfds: # Timeout
            return
        try:
            msg, blockAddr = self.blockSocket.recvfrom(MTU)
        except socket.error, e:
            if e.errno == socket.EBADF and self._continue == False:
                return # This happens during shutdown
        else: # Got a message
            if self.v:
                sys.stdout.write("BP recv %d from %s\n" % (len(msg), blockAddr))
            assert msg.startswith(self.SERIAL_HEADER)
            lenStart = len(self.SERIAL_HEADER)
            msgLength = struct.unpack('I', msg[lenStart:lenStart+4])
            message = msg[lenStart+4:]
            assert len(message) == msgLength
            if self.v:
                sys.stdout.write("\t%d[%d]" % (ord(message[0]), len(message)))
            self.clientSend(message) # Just pass it back to the phone


if __name__ == '__main__':
    # Test code for block proxy
    socket.socket(type=socket.SOCK_DGRAM).sendto(messages.FlashBlockIDs().serialize(), ("172.31.1.1", 5551))
