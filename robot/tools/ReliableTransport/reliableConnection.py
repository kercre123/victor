"""
Python replication of reliable connection class for Anki reliable transport prototcol.
Intended for prototyping and testing etc.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, struct, threading
from time import time
from ReliableTransport.reliableSequenceId import *

def GetCurrentTime():
    "Return current time in miliseconds"
    return time() * 1000.0

class PendingReliableMessage:
    "Data storage for pending reliable messages"

    def __init__(self, message, sequenceNumber, messageType, reliable=True):
        self.message = message
        self.sequenceNumber = sequenceNumber
        self.type = messageType

    def __repr__(self):
        return "PendingReliableMessage(%s, %d, %d, %s)" % (self.message, self.sequenceNumber, self.type)

class PingPayload(struct.Struct):
    "Serializable class for pings and pongs"

    def __init__(self, timestamp=0.0, numPingsSent=0, numPingsReceived=0, isReply=False, buffer=None, offset=0):
        struct.Struct.__init__(self, "dII?")
        self.timestamp        = timestamp
        self.numPingsSent     = numPingsSent
        self.numPingsReceived = numPingsReceived
        self.isReply          = isReply
        if buffer is not None:
            self.deserialize(buffer, offset)

    def serialize(self, buffer=None, offset=0):
        values = self.timestamp, self.numPingsSent, self.numPingsReceived, self.isReply
        if buffer is None:
            return self.pack(*values)
        else:
            return self.pack_into(buffer, offset, *values)

    def deserialize(self, buffer, offset=0):
        self.timestamp, self.numPingsSent, self.numPingsReceived, self.isReply = self.unpack_from(buffer, offset)

class ReliableConnection:
    "Information about a single reliable transport edge"

    PING_SEPERATION_TIME_MS            =  100.0
    UNACKED_MESSAGE_SEPERATION_TIME_MS =  100.0
    CONNECTION_TIMEOUT_MS              = 5000.0
    PING_ROUND_TRIP_FILTER = (0.1, 0.9) # New term, old term, must sum to 1
    assert sum(PING_ROUND_TRIP_FILTER) == 1.0

    def __init__(self, address):
        self.address = address
        self._nextOutSequenceId      = MIN_RELIABLE_SEQ_ID # Sequence ID for next outbound reliable message we send
        self._lastInAckedMessageId   = INVALID_RELIABLE_SEQ_ID # TRhe last messave we've acknowledged receiving
        self._nextInSequenceId       = MIN_RELIABLE_SEQ_ID
        self.latestUnackedMessageSentTime = 0.0
        self.latestRecvTime          = GetCurrentTime()
        self.latestPingSentTime      = GetCurrentTime()
        self.numPingsSent            = 0 # Pings we sent
        self.numPingsReceived        = 0 # Pings we've received
        self.numPingsSentThatArrived = 0 # Pings we sent that the other side received
        self.numPingsSentTowardsUs   = 0 # Pings that the other side claims to have sent
        self.pingRoundTripMovingAvg  = 0.0
        self.pendingMessageList = []

    def __repr__(self):
        return "ReliableConnection(%s)" % repr(self.address)

    def Print(self):
        return """%s: now=%f
    Out: %d unacked (%d..%d), next out %d, last send time %f
    In: Last Acked %d, Waiting for %d
    Ping RoundTrip %.3fms average. Last send at %f, %d of %d delivered. %d of %d received.
""" % (repr(self), GetCurrentTime(), len(self.pendingMessageList), self.FirstUnackedOutId, self.LastUnackedOutId,
        self._nextOutSequenceId, self.latestUnackedMessageSentTime,
        self._lastInAckedMessageId, self._nextInSequenceId,
        self.pingRoundTripMovingAvg, self.latestPingSentTime, self.numPingsSentThatArrived, self.numPingsSent,
        self.numPingsReceived, self.numPingsSentTowardsUs)

    def AddMessage(self, message, messageType, seqId):
        self.pendingMessageList.append(PendingReliableMessage(message, seqId, messageType))

    def GetNextOutSequenceNumber(self):
        nextSeqId = self._nextOutSequenceId
        self._nextOutSequenceId = NextSequenceId(self._nextOutSequenceId)
        assert ((self.GetLastUnackedOutId() == INVALID_RELIABLE_SEQ_ID) or \
                (self.GetLastUnackedOutId() == PreviousSequenceId(nextSeqId)))
        assert (nextSeqId != INVALID_RELIABLE_SEQ_ID)
        assert (nextSeqId >= MIN_RELIABLE_SEQ_ID)
        assert (nextSeqId <= MAX_RELIABLE_SEQ_ID)
        assert (not IsSequenceIdInRange(nextSeqId, self.GetFirstUnackedOutId(), self.GetLastUnackedOutId())) # Reusing an ack that's still pending!
        return nextSeqId

    def IsWaitingForAnyInRange(self, minSeqId, maxSeqId):
        return IsSequenceIdInRange(self._nextInSequenceId, minSeqId, maxSeqId)

    def IsNextInSequenceId(self, seqNum):
        return self._nextInSequenceId == seqNum

    @property
    def NextInSequenceId(self):
        return self._nextInSequenceId

    def AdvanceNextInSequenceId(self):
        self._nextInSequenceId = NextSequenceId(self._nextInSequenceId)

    def UpdateLastAckedMessage(self, seqId):
        #print("%s UpdateLastAckedMessage to %d" % (threading.currentThread().name, seqId))
        updated = False
        if seqId != INVALID_RELIABLE_SEQ_ID and len(self.pendingMessageList) > 0:
            while IsSequenceIdInRange(seqId, self.GetFirstUnackedOutId(), self.GetLastUnackedOutId()):
                self.pendingMessageList.pop(0)
                updated = True
        self.latestRecvTime = GetCurrentTime()
        return updated

    def GetFirstUnackedOutId(self):
        if len(self.pendingMessageList) == 0:
            return INVALID_RELIABLE_SEQ_ID
        else:
            return self.pendingMessageList[0].sequenceNumber

    def GetLastUnackedOutId(self):
        if len(self.pendingMessageList) == 0:
            return INVALID_RELIABLE_SEQ_ID
        else:
            return self.pendingMessageList[-1].sequenceNumber

    def SendPing(self, transport, incomingPingTime=0.0, isReply=False):
        self.numPingsSent += 1
        currentTime = GetCurrentTime()
        pingTime = incomingPingTime if isReply else currentTime
        ping = PingPayload(pingTime, self.numPingsSent, self.numPingsReceived, isReply)
        transport.SendMessage(True, self.address, ping.serialize(), EReliableMessageType.Ping)
        if not isReply:
            self.latestPingSentTime = currentTime

    def ReceivePing(self, transport, buffer):
        currentTime = GetCurrentTime()
        ping = PingPayload(buffer=buffer)
        self.numPingsReceived += 1
        # Pings can arrive out of order
        if ping.numPingsSent > self.numPingsSentTowardsUs:
            assert ping.numPingsReceived >= self.numPingsSentThatArrived
            self.numPingsSentTowardsUs   = ping.numPingsSent
            self.numPingsSentThatArrived = ping.numPingsReceived
        if ping.isReply:
            if currentTime < ping.timestamp:
                sys.stderr.write("ReliableConnection: Ping has returned before it was sent! (currentTime %f vs ping sent %f)\n" % (currentTime, ping.timestamp))
            roundTripTime = currentTime - ping.timestamp
            self.pingRoundTripMovingAvg = (roundTripTime               * self.PING_ROUND_TRIP_FILTER[0]) + \
                                          (self.pingRoundTripMovingAvg * self.PING_ROUND_TRIP_FILTER[1])
        else: # Send reply
            self.SendPing(transport, ping.timestamp, True)

    def AckMessage(self, seqId):
        #print("%s ACK to %d" % (threading.currentThread().name, seqId))
        assert seqId != INVALID_RELIABLE_SEQ_ID
        self._lastInAckedMessageId = seqId

    def SendUnAckedMessages(self, transport, newestFirst, firstToSend=0):
        maxPayloadPerMessage = transport.maxTotalBytesPerMessage
        numMessagesToSend = 0
        numBytesToSend = 0
        k = 0
        # Figure out which messages we will send
        for i in range(firstToSend, len(self.pendingMessageList)):
            k = len(self.pendingMessageList) - 1 - i if newestFirst else i
            numBytesToSendIfAdded = numBytesToSend + len(self.pendingMessageList[k].message)
            # Conditionally add appropriate sub message header or headers
            if numMessagesToSend == 0: # Considering first message
                pass
            elif numMessagesToSend == 1: # Considering adding the second message
                numBytesToSendIfAdded += 2*MultiPartSubMessageHeader.LENGTH
            else: # Considering adding the third and thereafter message
                numBytesToSendIfAdded += MultiPartSubMessageHeader.LENGTH
            # Is there room to add this to the packet?
            if numBytesToSendIfAdded <= maxPayloadPerMessage:
                numMessagesToSend += 1
                numBytesToSend = numBytesToSendIfAdded
            else:
                break
        # Not sending anything
        if numMessagesToSend == 0:
            return 0
        # Now that we know what to send, pack it
        if newestFirst:
            loInd = len(self.pendingMessageList)-firstToSend-numMessagesToSend
            hiInd = len(self.pendingMessageList)-firstToSend
        else:
            loInd = firstToSend
            hiInd = firstToSend+numMessagesToSend
        assert loInd >= 0
        assert loInd <= hiInd
        assert hiInd <= len(self.pendingMessageList)
        seqIdMin = INVALID_RELIABLE_SEQ_ID
        seqIdMax = INVALID_RELIABLE_SEQ_ID
        for i in range(loInd, hiInd, +1):
            if self.pendingMessageList[i].sequenceNumber != INVALID_RELIABLE_SEQ_ID:
                seqIdMin = self.pendingMessageList[i].sequenceNumber
                break
        for i in range(hiInd-1, loInd-1, -1):
            if self.pendingMessageList[i].sequenceNumber != INVALID_RELIABLE_SEQ_ID:
                seqIdMax = self.pendingMessageList[i].sequenceNumber
                break
        if numMessagesToSend == 1:
            transport.ReSendReliableMessage(self, self.pendingMessageList[k].message, self.pendingMessageList[k].type, \
                                            seqIdMin, seqIdMax)
            if self.pendingMessageList[k].type in UNRELIABLE_MESSAGE_TYPES:
                self.pendingMessageList.pop(k)
        else:
            buffer = b""
            toRemove = []
            for i in range(loInd, hiInd):
                pm = self.pendingMessageList[i]
                buffer += MultiPartSubMessageHeader(pm.type, len(pm.message)).serialize() + pm.message
                if pm.type in UNRELIABLE_MESSAGE_TYPES:
                    toRemove.append(pm)
            for pm in toRemove:
                self.pendingMessageList.remove(pm) # Remove all the unreliable messages from the queue
            assert len(buffer) == numBytesToSend, (len(buffer), numBytesToSend)
            transport.ReSendReliableMessage(self, buffer, EReliableMessageType.MultipleMixedMessages, seqIdMin, seqIdMax)
        self.latestUnackedMessageSentTime = GetCurrentTime()
        return numMessagesToSend

    @property
    def HasConnectionTimedOut(self):
        return GetCurrentTime() > (self.latestRecvTime + self.CONNECTION_TIMEOUT_MS)

    def SendUnAckedPackets(self, transport, maxPacketsToSend, newestFirst, firstToSend):
        numPacketsSent = 0
        while numPacketsSent < maxPacketsToSend:
            numMessagesSentThisPacket = self.SendUnAckedMessages(transport, newestFirst, firstToSend)
            if numMessagesSentThisPacket == 0:
                break
            else:
                firstToSend += numMessagesSentThisPacket
                numPacketsSent += 1
        return numPacketsSent

    def Update(self, transport):
        currentTime = GetCurrentTime()
        # Send any un-acked messages up to maximum number of packets
        if currentTime > self.latestUnackedMessageSentTime + self.UNACKED_MESSAGE_SEPERATION_TIME_MS:
            self.SendUnAckedPackets(transport, 3, False, 0)
        # Send ping if it's been too long since we've sent anything
        if currentTime > self.latestPingSentTime + self.PING_SEPERATION_TIME_MS:
            self.SendPing(transport)
        # Return status
        return not self.HasConnectionTimedOut

    @property
    def FirstUnackedOutId(self):
        if len(self.pendingMessageList) == 0:
            return INVALID_RELIABLE_SEQ_ID
        else:
            return self.pendingMessageList[0].sequenceNumber

    @property
    def LastUnackedOutId(self):
        if len(self.pendingMessageList) == 0:
            return INVALID_RELIABLE_SEQ_ID
        else:
            return self.pendingMessageList[-1].sequenceNumber

    @property
    def LastInAckedMessageId(self):
        return self._lastInAckedMessageId
