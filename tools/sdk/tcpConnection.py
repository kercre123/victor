"""
Utility classes for interfacing with the Cozmo engine through a localhost TCP socket, by exchanging CLAD messages.
"""
__author__ = "Mark Wesley"

import sys, os
import socket
import struct


class TcpConnection: #socket.socket):


    kMaxBytesToReadFromSocket = 4096
    MAX_MESSAGE_SIZE = 2046  # individual messages aren't expected beyond 2048 the other side, and we add 2 bytes to message for the size

    SDK_ON_COMPUTER_TCP_PORT = 5107


    def __init__(self, ipAddress='127.0.0.1', port=SDK_ON_COMPUTER_TCP_PORT):
        self.socket = None
        self.addr = (ipAddress, port)
        self.isConnected = False
        self.recvBuffer = bytes()
        self.Reconnect()
        

    def Update(self):
        "Maintain connection info, try to reconnect if not currently connected"
        if not self.isConnected:
            self.Connect()
        

    def Connect(self):
        "Connect to TCP Socket"
        if self.isConnected:
            sys.stderr.write("[TcpConnection.Connect] Already connected!? " + str(self.addr) + "'" + os.linesep)
        try:
            self.socket.connect(self.addr)
            sys.stdout.write("[TcpConnection.Connect] Connected to '" + str(self.addr) + "'" + os.linesep)
            self.isConnected = True
        except Exception as e:
            sys.stderr.write("[TcpConnection.Connect] Error connecting to '" + str(self.addr) + "': " + str(e) + os.linesep)
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 22] Invalid argument
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 9] Bad file descriptor (after closing socket)
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 36] Operation now in progress
            # [TcpConnection.Connect] Error connecting to '('127.0.0.1', 5107)': [Errno 56] Socket is already connected


    def Disconnect(self):
        self.isConnected = False
        if self.socket != None:
            self.socket.close()
            self.socket = None


    def Reconnect(self):
        self.Disconnect()
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.setsockopt(socket.SOL_TCP, socket.TCP_NODELAY, 1)
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        # TODO - see if TCP_QUICKACK is usable on this socket
        self.Connect()
        # Make the socket non-blocking (doesn't seem to connect if we do that before Connect())
        self.socket.setblocking(False)
        self.socket.settimeout(0)


    def CloseSocket(self):
        "Close the underlying socket"
        return self.socket.close()


    def SendData(self, message):
        "send"
        if not self.isConnected:
            return False

        if len(message) > self.MAX_MESSAGE_SIZE:
            raise ValueError("[TcpConnection.SendData] Max message size for TcpConnection is %d, given %d" % (self.MAX_MESSAGE_SIZE, len(message)))
        else:
            try:
                subMessageSize = struct.pack('H', len(message))
                # TODO: Combine into one send?
                self.socket.send(subMessageSize)
                self.socket.send(message)
            except Exception as e:    
                sys.stderr.write("[TcpConnection.SendData] Error: " + str(e) + os.linesep)
                #[TcpConnection.SendData] Error: [Errno 32] Broken pipe
                self.Reconnect()


    def ReceiveDataFromSocket(self):
        if not self.isConnected:
            return None

        try:
            data = self.socket.recv(self.kMaxBytesToReadFromSocket)
        except Exception as e:
            exceptionAsString = str(e)
            if exceptionAsString == "[Errno 35] Resource temporarily unavailable":
                # Expected on non-blocking sockets whenever there is no data to read
                pass
            else:
                sys.stderr.write("[TcpConnection.ReceiveDataFromSocket] ReceiveData error: " + exceptionAsString + os.linesep)
                # e.g: [TcpConnection.ReceiveDataFromSocket] ReceiveData error: [Errno 54] Connection reset by peer
                #      [Errno 32] Broken pipe
                self.Reconnect()

            return None
        else:
            return data


    def ReceiveData(self):
        """Receive a packet if any
        If a packet is available returns data, sourceAddres. Otherwise returns None, None
        Tries buffer first (for a message)"""

        data = self.ReceiveDataFromSocket()

        if (data != None):
            self.recvBuffer += data

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
                return data, self.addr
        
        return None, None 

