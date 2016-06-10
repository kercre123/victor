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
        self.lastMsgRecvTime_s = 0.0
        self.hasConnectionTimedOut = False
        self.connectionTimeoutLimit_s = 5.0

    def IsConnected(self):
        "IsConnected: Only true if fully connected (with ack)"
        return self.connectionState == ConnectionState.Connected

    def OnConnectionAcked(self):
        self.connectionState = ConnectionState.Connected

    def _FilterReceivedMessage(self, data, addr):
        "Called when receiving a message, also provides a potential mechanism for filtering/discarding internal messages"
        if self.hasConnectionTimedOut:
            timeOutDuration_s = time.time() - self.lastMsgRecvTime_s
            sys.stdout.write("Recovered after long timeout ({0:.1f} s){ls}".format(timeOutDuration_s, ls=os.linesep))
            self.hasConnectionTimedOut = False;
        self.lastMsgRecvTime_s = time.time()
        return data, addr

    def Update(self):
        "Maintain connection info, try to reconnect if not currently connected"
        # Timeout the connection if we don't hear anything back from the engine in connectionTimeoutLimit_s seconds 
        if not self.hasConnectionTimedOut and (self.lastMsgRecvTime_s > 0.0):
            timeSinceLastMsg_s = time.time() - self.lastMsgRecvTime_s
            if (timeSinceLastMsg_s > self.connectionTimeoutLimit_s):                
                self.hasConnectionTimedOut = True
                sys.stdout.write("Detected long time since last received message ({0:.1f} s){ls}".format(timeSinceLastMsg_s, ls=os.linesep))
                # We could automatically ResetConnection here which would then attempt to reconnect later, but
                # whilst debugging etc. it's expected to get a timeout that we later recover from, so we stay connected
                # but provide a manual reconnect method for the user in the event of e.g. the engine restarting
                #self.engineInterface.ResetConnection()

    def HandlePingMessage(self, msg):
        if not msg.isResponse:
            # Send it back to the engine so it can calculate latency
            self._SendPingResponse(msg)
        else:
            #latency = (1000.0 * time.time()) - msg.timeSent_ms
            pass

    def _SendPing(self):
        "Send a Ping message to the engine with the current time"
        outPingMsg = self.GToEI.Ping()
        outPingMsg.counter = self.pingCount 
        outPingMsg.timeSent_ms = (1000.0 * time.time())
        outPingMsg.isResponse = False
        toEngMsg = self.GToEM(Ping = outPingMsg)
        self.SendMessage(toEngMsg, False)
        self.pingCount += 1

    def _SendPingResponse(self, msg):
        "When receiving a ping from the engine, reply with an identical response (so that that engine can calculate latency etc.)"
        outPingMsg = self.GToEI.Ping()
        outPingMsg.counter = msg.counter
        outPingMsg.timeSent_ms = msg.timeSent_ms
        outPingMsg.isResponse = True
        toEngMsg = self.GToEM(Ping = outPingMsg)
        self.SendMessage(toEngMsg, False)

    def Disconnect(self):
        "Disconnect from any subclass's underlying network connection and reset any connection state"
        self.connectionState = ConnectionState.NotConnected
        self.lastMsgRecvTime_s = 0.0
        self.hasConnectionTimedOut = False

    def ResetConnection(self):
        "Reset the connection and allow it to attempt to reconnect on future updates"
        raise NotImplementedError

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

