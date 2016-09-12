"""
Manages a connection to Cozmo engine through a localhost UDP socket, by exchanging CLAD messages
"""
__author__ = "Mark Wesley"


import os
import socket
import sys
from iNetConnection import INetConnection, ConnectionState
from verboseLevel import VerboseLevel


class UdpConnection(INetConnection): 
    "Wrapper around a UDP socket.socket"
    # [MARKW:TODO] Make it maintain a connection (i.e. reconnect after detecting a timed-out detection)

    MTU = 1500
    MAX_MESSAGE_SIZE = 1472  # Assuming a an MTU of 1500, minus (20+8) for the underlying IP and UDP headers, this should be optimal for most networks/users


    def __init__(self, verboseLevel, incomingPort, GToEI, GToEM):
        super().__init__(verboseLevel, GToEI, GToEM)
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.socket.settimeout(0) # Make the socket non-blocking
        self.socket.bind(('', incomingPort))
        self.adMsg = None
        self.adDest = None
        self.engDest = None

    def Update(self):
        "Maintain connection info, try to reconnect if not currently connected"
        super().Update()
        if not self.IsConnected():
            self._TryToConnect()

    def _TryToConnect(self):
        "Try To Connect over UDP Socket"
        if (self.connectionState == ConnectionState.NotConnected): 
            sys.stdout.write("Trying to connect to Engine..." + os.linesep);
            self.connectionState = ConnectionState.WaitingForAck

        if (self.connectionState == ConnectionState.WaitingForAck):            
            if self.verboseLevel >= VerboseLevel.Max:
                sys.stdout.write("[UdpConnection._TryToConnect] Sending adMsg '" + str(self.adMsg) + "'" + os.linesep)
            self._SendData(self.adDest, self.adMsg.pack())
        else:
            sys.stderr.write("[UdpConnection._TryToConnect] Unexpected state: " + str(self.connectionState) + os.linesep)

    def Disconnect(self):
        "Disconnect from underlying network connection and reset any state"
        self._CloseSocket()
        super().Disconnect()

    def ResetConnection(self):
        "Reset the connection and allow it to attempt to reconnect on future updates"
        # In this case, we do not need to completely close the socket and recreate a connection,
        # we can just begin searching again for a connection on the existing socket
        super().Disconnect()

    def _CloseSocket(self):
        "Close the underlying socket"
        return self.socket.close()

    def _SendData(self, destAddress, messageBytes):
        "Internal implementation for sending bytes over the connection to destAddress"
        lenMessageBytes = len(messageBytes)
        if lenMessageBytes > self.MAX_MESSAGE_SIZE:
            raise ValueError("Max message size for UdpConnection is %d, given %d" % (self.MAX_MESSAGE_SIZE, len(messageBytes)))
        else:
            self.socket.sendto(messageBytes, destAddress)

    def SendBytes(self, messageBytes):
        "Send messageBytes to the connection's desination"
        try:
            sendRes = self._SendData(self.engDest, messageBytes)
            return sendRes
        except Exception as e:
            sys.stderr.write("[UdpConnection.SendMessage] Exception sending data: " + str(e) + os.linesep)
            return False

    def ReceiveMessage(self):
        "returns 'messageData, sourceAddres' if there is a message to receive, 'None, None' otherwise"
        try:
            data, addr = self.socket.recvfrom(self.MTU)
            return self._FilterReceivedMessage(data, addr)
        except:
            return None, None

