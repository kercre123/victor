"""
Interface and some Common Code for all network connections
"""
__author__ = "Mark Wesley"


import os
import sys
import time
from verboseLevel import VerboseLevel


class ConnectionState:
    "An enum for connection states"
    NotConnected    = 0
    WaitingForAck   = 1
    Connected       = 2


class INetConnection:
    "Base Interface for all network connections"    

    def __init__(self, verboseLevel, GToEI, GToEM):
        self.connectionState = ConnectionState.NotConnected
        self.verboseLevel = verboseLevel
        self.GToEI = GToEI
        self.GToEM = GToEM
        self.pingCount = 0

    def IsConnected(self):
        "IsConnected: Only true if fully connected (with ack)"
        return self.connectionState == ConnectionState.Connected

    def OnConnectionAcked(self):
        self.connectionState = ConnectionState.Connected

    def Update(self):
        "Maintain connection info, try to reconnect if not currently connected"
        # [MARKW:TODO] Check most recent receive time, and timeout connection if over a reasonable limit (e.g. 5 seconds)
        if self.IsConnected():
            self.SendPing()

    def SendPing(self):
        outPingMsg = self.GToEI.Ping()
        outPingMsg.counter = self.pingCount 
        outPingMsg.timeSent = time.time()
        outPingMsg.isResponse = False
        toEngMsg = self.GToEM(Ping = outPingMsg)
        self.SendMessage(toEngMsg, False)
        self.pingCount += 1

    def Disconnect(self):
        "Disconnect from any subclass's underlying network connection and reset any connection state"
        self.connectionState = ConnectionState.NotConnected

    def SendBytes(self, messageBytes):
        "Send messageBytes to the connection's desination"
        raise NotImplementedError

    def SendMessage(self, message, logIfVerbose=True):
        "SendMessage via the subclass's SendBytes Implementation"
        if (self.verboseLevel >= VerboseLevel.Max) or ((self.verboseLevel >= VerboseLevel.High) and logIfVerbose):
            try:
                sys.stdout.write("["+self.__class__.__name__+".SendMessage] '" + str(message) + "'" + os.linesep)
            except Exception as e:
                sys.stderr.write("["+self.__class__.__name__+".SendMessage] Exception attempting to log message: " + str(e) + os.linesep)        
        try:
            messageBytes = message.pack()
        except Exception as e:
            sys.stderr.write("["+self.__class__.__name__+".SendMessage] Exception packing message: " + str(e) + os.linesep)
            return False
        return self.SendBytes(messageBytes)

    def ReceiveMessage(self):
        "returns 'messageData, sourceAddres' if there is a message to receive, 'None, None' otherwise"
        raise NotImplementedError

