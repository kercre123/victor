"""
Manages a connection to Cozmo engine through a TCP socket, by exchanging CLAD messages
"""
__author__ = "Mark Wesley"


import os
import socket
import struct
import sys
from iNetConnection import INetConnection, ConnectionState
from verboseLevel import VerboseLevel


class TcpConnection(INetConnection): 
    "Wrapper around a TCP socket.socket, maintains connection and prepends each message with size for easy per-message deserialization"

    MAX_BYTES_TO_READ_FROM_SOCKET = 4096
    MAX_MESSAGE_SIZE = 2046  # individual messages aren't expected beyond 2048 the other side, and we add 2 bytes to message for the size

    def __init__(self, verboseLevel, ipAddress, port, GToEI, GToEM):
        super().__init__(verboseLevel, GToEI, GToEM)
        self.socket = None
        self.addr = (ipAddress, port)
        self.recvBuffer = bytes()
        self._Reconnect()

    def Update(self):
        "Maintain connection info, try to reconnect if not currently connected"
        super().Update()
        if self.connectionState == ConnectionState.NotConnected:
            self._TryToConnect()
        
    def _TryToConnect(self):
        "Try To Connect to TCP Socket"
        if self.connectionState >= ConnectionState.WaitingForAck:
            sys.stderr.write("[TcpConnection._TryToConnect] Already connected!? " + str(self.addr) + "'" + os.linesep)
        try:
            self.socket.connect(self.addr)
            sys.stdout.write("[TcpConnection._TryToConnect] Connected to '" + str(self.addr) + "'" + os.linesep)
            self.connectionState = ConnectionState.WaitingForAck
        except Exception as e:
            sys.stderr.write("[TcpConnection._TryToConnect] Error connecting to '" + str(self.addr) + "': " + str(e) + os.linesep)
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 22] Invalid argument
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 9] Bad file descriptor (after closing socket)
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 36] Operation now in progress
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 56] Socket is already connected

    def Disconnect(self):
        "Disconnect from underlying network connection and reset any state"
        self._CloseSocket()
        super().Disconnect()

    # We must ensure we destroy the TCP connection otherwise we will dangle the socket
    def ResetConnection(self):
        "Reset the connection and allow it to attempt to reconnect on future updates"
        self._Reconnect()

    def _Reconnect(self):
        self.Disconnect()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # TODO - see if TCP_QUICKACK is usable on this socket
        self._TryToConnect()
        # Make the socket non-blocking (doesn't seem to connect if we do that before Connect())
        self.socket.setblocking(False)
        self.socket.settimeout(0)

    def _CloseSocket(self):
        "Close the underlying socket"
        if self.socket != None:
            self.socket.close()
            self.socket = None
        self.recvBuffer = bytes()

    def _SendData(self, messageBytes):
        "Internal implementation for sending bytes over the connection"
        if not self.IsConnected():
            return False
        lenMessageBytes = len(messageBytes)
        if lenMessageBytes > self.MAX_MESSAGE_SIZE:
            raise ValueError("[TcpConnection.SendData] Max message size for TcpConnection is %d, given %d" % (self.MAX_MESSAGE_SIZE, lenMessageBytes))
        else:
            try:
                subMessageSize = struct.pack('H', lenMessageBytes)
                # TODO: Combine into one send?
                self.socket.send(subMessageSize)
                self.socket.send(messageBytes)
            except Exception as e:    
                sys.stderr.write("[TcpConnection.SendData] Error: " + str(e) + os.linesep)
                #[TcpConnection.SendData] Error: [Errno 32] Broken pipe
                self._Reconnect()

    def SendBytes(self, messageBytes):
        "Send messageBytes to the connection's desination"
        try:
            sendRes = self._SendData(messageBytes)
            return sendRes
        except:
            sys.stderr.write("[TcpConnection.SendBytes] Exception sending data: " + str(e) + os.linesep)
            return False

    def _ReceiveDataFromSocket(self):
        "Try to recv on the socket, append recvBuffer with any bytes received"
        # We attempt to receive in WaitingForAck as well as fully connected
        if self.connectionState < ConnectionState.WaitingForAck:
            return False
        try:
            data = self.socket.recv(self.MAX_BYTES_TO_READ_FROM_SOCKET)
        except Exception as e:
            # [MARKW:TODO] There must be a better way to catch specific exception codes
            exceptionAsString = str(e)
            if exceptionAsString == "[Errno 35] Resource temporarily unavailable":
                # Expected on non-blocking sockets whenever there is no data to read
                pass
            else:
                sys.stderr.write("[TcpConnection.ReceiveDataFromSocket] ReceiveData error: " + exceptionAsString + os.linesep)
                # e.g: [TcpConnection.ReceiveDataFromSocket] ReceiveData error: [Errno 54] Connection reset by peer
                #      [Errno 32] Broken pipe
                self._Reconnect()
        else:
            if (data != None):
                self.recvBuffer += data
                return True
        return False

    def ReceiveMessage(self):
        "returns 'messageData, sourceAddres' if there is a message to receive, 'None, None' otherwise"

        # First pull any additional available bytes from the socket
        self._ReceiveDataFromSocket()

        # Need at least 2 bytes in the buffer as the first 2 store the message length
        bytesLeft = len(self.recvBuffer)

        if (bytesLeft >= 2):
            try:
                messageSize = struct.unpack_from('H', self.recvBuffer[0:] )
                messageSize = messageSize[0]
            except Exception as e:
                sys.stderr.write("[TcpConnection.ReceiveData] Error unpacking: " + str(e) + os.linesep)
                return None, None

            if messageSize <= (bytesLeft - 2):
                # We have the whole message
                data = self.recvBuffer[2:2+messageSize]
                self.recvBuffer = self.recvBuffer[2+messageSize: ]
                return self._FilterReceivedMessage(data, self.addr)
        
        return None, None 

