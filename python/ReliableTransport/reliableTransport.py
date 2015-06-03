"""
Python implementation of Anki (Cozmo) reliable transport protocol for prototyping, testing and scripting. This is based
on the Anki Drive driveEngine/networkService/reliableTransport layer but with some customization for compatibility
with embedded constraints and Cozmo's needs.
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import sys, struct, ctypes, threading, select
from ReliableTransport.reliableSequenceId import *
from ReliableTransport.reliableConnection import ReliableConnection

class IDataReceiver:
    def OnConnectionRequest(self, sourceAddress):
        raise Exception("IDataReceiver subclasses must implement their own OnConnectionRequest callback")
    def OnConnected(self, sourceAddress):
        raise Exception("IDataReceiver subclasses must implement their own OnConnected callback")
    def OnDisconnected(self, sourceAddress):
        raise Exception("IDataReceiver subclasses must implement their own OnDisconnected callback")
    def ReceiveData(self, buffer, sourceAddress):
        raise Exception("IDataReceiver subclasses must implement their own ReceiveData callback")

class IUnreliableTransport:
    PACKET_HEADER = b'COZ\x03'

    def OpenSocket(self, port, interface):
        return True # Optional to implement
    def CloseSocket(self):
        return True # Optional to implement
    def SendData(self, destAddress, message):
        raise Exception("IUnreliableTransport subclasses must implement their own SendData method")
    def ReceiveData(self):
        raise Exception("IUnreliableTransport subclasses must implement their own ReceiveData method")
    def __del__(self):
        self.CloseSocket()

class AnkiReliablePacketHeader(struct.Struct):
    "Header for reliable transport packets"

    LENGTH = 10

    PREFIX = b"RE\x01"

    def __init__(self, messageType=EReliableMessageType.Invalid, \
                       seqIdMin=INVALID_RELIABLE_SEQ_ID, seqIdMax=INVALID_RELIABLE_SEQ_ID, \
                       lastReceivedId=INVALID_RELIABLE_SEQ_ID, buffer=None, offset=0):
        struct.Struct.__init__(self, "3sBHHH")
        self.type = messageType
        self.seqIdMin = seqIdMin
        self.seqIdMax = seqIdMax
        self.lastReceivedId = lastReceivedId
        if buffer is not None:
            self._deserialize(buffer, offset)

    def serialize(self, buffer=None, offset=0):
        "Serialize the header for the wire"
        values = self.PREFIX, self.type, self.seqIdMin, self.seqIdMax, self.lastReceivedId
        if buffer is None:
            return self.pack(*values)
        else:
            return self.pack_into(buffer, offset, *values)

    def __len__(self):
        "Return the structure wire size"
        assert self.size == self.LENGTH
        return self.LENGTH

    def _deserialize(self, buffer, offset=0):
        "Deserialize structre from network format"
        assert self.HasCorrectPrefix(buffer[offset:])
        prefix, self.type, self.seqIdMin, self.seqIdMax, self.lastReceivedId = self.unpack_from(buffer, offset)

    @classmethod
    def HasCorrectPrefix(cls, buffer):
        "Check if a buffer has the right prefix to be a reliable packet header"
        return buffer.startswith(cls.PREFIX)

    def __repr__(self):
        return "AnkiReliablePacketHeader(%d, %d, %d, %d)" % (self.type, self.seqIdMin, self.seqIdMax, self.lastReceivedId)

class ReliableTransport(threading.Thread):
    "Class for managing reliable transport streams"

    IMMEDIATE_ACK   = False # Property, whether to ack immediately when receiving messages

    def __init__(self, unreliable, receiver):
        "Set up a new reliable transport manager given an unreliable transport layer and a receiver callback object"
        threading.Thread.__init__(self)
        self.unreliable = unreliable
        self.receiver   = receiver
        self.queue = []
        self.lock = threading.Lock()
        self._continue = True
        self.connections = {}
        self.pktLog = open('rx-packet-log.txt', 'w')

    def __del__(self):
        self.KillThread()
        self.ClearConnections()
        self.pktLog.close()

    def run(self):
        while self._continue:
            workToDo = self.lock.acquire(False) # Don't block on mutex, just declare false
            if workToDo:
                workToDo = len(self.queue) > 0
                self.lock.release()
            else:
                rlist, wlist, xlist = select.select([self.unreliable], [], [], 0.100)
            self.Update()

    def KillThread(self):
        "Clean up the thread"
        self._continue = False
        self.lock.acquire()
        self.queue = []
        self.lock.release()

    def ClearConnections(self):
        "Clear the connections"
        for k in list(self.connections.keys()):
            del self.connections[k]

    @property
    def maxTotalBytesPerMessage(self):
        "Maximum payload size allowing for header"
        return self.unreliable.MAX_MESSAGE_SIZE - AnkiReliablePacketHeader.LENGTH

    def ReSendReliableMessage(self, connectionInfo, buffer, messageType, seqIdMin, seqIdMax):
        "Send messages which have already been sent"
        pkt = AnkiReliablePacketHeader(messageType, seqIdMin, seqIdMax, connectionInfo.LastInAckedMessageId).serialize() + buffer
        assert len(pkt) <= self.maxTotalBytesPerMessage, pkt
        self.unreliable.SendData(connectionInfo.address, pkt)

    def QueueMessage(self, *args):
        "Thread safe version of SendMessage"
        self.lock.acquire()
        self.queue.append((self.SendMessage, args))
        self.lock.release()

    def SendMessage(self, hot, destAddress, buffer, messageType):
        "Send a message over UDP"
        assert len(buffer) <= self.maxTotalBytesPerMessage # We don't support multi-part messages in this implementation
        # Find the connection associated with this destination, or make a new one
        connectionInfo = self.FindConnection(destAddress, True)
        if messageType in RELIABLE_MESSAGE_TYPES:
            seqId = connectionInfo.GetNextOutSequenceNumber()
        elif messageType in UNRELIABLE_MESSAGE_TYPES:
            seqId = INVALID_RELIABLE_SEQ_ID
        else:
            raise ValueError("Message type must be in either RELIABLE_MESSAGE_TYPES or UNRELIABLE_MESSAGE_TYPES")
        # Add header data and pack up the message
        connectionInfo.AddMessage(buffer, messageType, seqId)
        if hot:
            connectionInfo.SendUnAckedMessages(self, True)

    def SendData(self, reliable, hot, destAddress, buffer):
        "Wrapps data in a message and queues for sending"
        msgType = EReliableMessageType.SingleReliableMessage if reliable else EReliableMessageType.SingleUnreliableMessage
        self.QueueMessage(hot, destAddress, buffer, msgType)

    def Connect(self, destAddress):
        "Send a connection request to a remote address to establish mutual edges"
        self.QueueMessage(True, destAddress, b"", EReliableMessageType.ConnectionRequest)

    def FinishConnection(self, destAddress):
        "Respond to a connection request"
        self.QueueMessage(True, destAddress, b"", EReliableMessageType.ConnectionResponse)

    def Disconnect(self, destAddress):
        "Request disconnection from remote end"
        self.QueueMessage(true, destAddress, b"", EReliableMessageType.DisconnectRequest)

    def HandleSubMessage(self, innerMessage, messageType, seqId, connectionInfo, sourceAddress):
        "Handle a single message from inside a multi-part message"
        if messageType in RELIABLE_MESSAGE_TYPES:
            if connectionInfo.IsNextInSequenceId(seqId):
                connectionInfo.AdvanceNextInSequenceId()
            else:
                #sys.stdout.write("Ignoring out of order / duplicate message seqNo %d, type %d from %s\r\n" % (seqId, messageType, sourceAddress))
                return
        # Switch on message type
        if   messageType == EReliableMessageType.ConnectionRequest:
            if self.receiver:
                self.receiver.OnConnectionRequest(sourceAddress)
        elif messageType == EReliableMessageType.ConnectionResponse:
            if self.receiver:
                self.receiver.OnConnected(sourceAddress)
        elif messageType == EReliableMessageType.DisconnectRequest:
            if self.receiver:
                self.receiver.OnDisconnected(sourceAddress)
        elif messageType == EReliableMessageType.SingleReliableMessage:
            if self.receiver:
                self.receiver.ReceiveData(innerMessage, sourceAddress)
        elif messageType == EReliableMessageType.SingleUnreliableMessage:
            if self.receiver:
                self.receiver.ReceiveData(innerMessage, sourceAddress)
        elif messageType == EReliableMessageType.MultiPartMessage:
            raise ValueError("Multi-part messages not handled by this implementation")
        elif messageType in (EReliableMessageType.MultipleReliableMessages, EReliableMessageType.MultipleUnreliableMessages, EReliableMessageType.MultipleMixedMessages):
            raise ValueError("MultipleMessages flag should have been handled by ReceiveData not passed here")
        elif messageType == EReliableMessageType.ACK:
            pass # Already handled for all messages from the header earlier, nothing left to do
        elif messageType == EReliableMessageType.Ping:
            connectionInfo.ReceivePing(self, innerMessage)
        else:
            raise ValueError("Unknown message type " + str(messageType))

    def ReceiveData(self):
        "Handle receiving raw bytes from unreliable transport"
        buffer, sourceAddress = self.unreliable.ReceiveData()
        if buffer is None:
            return
        handledMessageType = False
        self.pktLog.write(" ".join(["%02x" % b for b in buffer]) + "\n")
        self.pktLog.flush()
        if (len(buffer) < AnkiReliablePacketHeader.LENGTH):
            sys.stderr.write("ReceiveData too small %d\r\n" % len(buffer))
        else:
            try:
                reliablePacketHeader = AnkiReliablePacketHeader(buffer=buffer)
            except:
                sys.stderr.write("ReceiveData incorrect header: %02x%02x%02x%02x" % tuple(buffer[:4]))
            else:
                connectionInfo = self.FindConnection(sourceAddress, True)
                handledMessageType = True
                # Tell the reliability layer any messages the other end has received
                if connectionInfo.UpdateLastAckedMessage(reliablePacketHeader.lastReceivedId):
                    # The list of reliable messages was updated, if we have more data, send it
                    connectionInfo.SendUnAckedPackets(self, 2, False, 0) # Why two packets?
                if reliablePacketHeader.seqIdMax != INVALID_RELIABLE_SEQ_ID:
                    connectionInfo.AckMessage(reliablePacketHeader.seqIdMax)
                    if self.IMMEDIATE_ACK:
                        print(threading.currentThread().name, "Sending ACK")
                        self.SendMessage(True, sourceAddress, b"", EReliableMessageType.ACK)
                # Unpack payload
                innerMessage = buffer[reliablePacketHeader.LENGTH:]
                if reliablePacketHeader.type in (EReliableMessageType.MultipleReliableMessages, \
                                                 EReliableMessageType.MultipleUnreliableMessages, \
                                                 EReliableMessageType.MultipleMixedMessages):
                    seqIdCounter = reliablePacketHeader.seqIdMin
                    bytesProcessed = 0
                    while bytesProcessed < len(innerMessage):
                        try:
                            subMessageHeader = MultiPartSubMessageHeader(buffer=innerMessage, offset=bytesProcessed)
                        except:
                            sys.stderr.write("ReceiveData couldn't unpack submessage header: %02x%02x%02x\r\n" % \
                                             tuple(buffer[bytesProcessed:bytesProcessed+3]))
                            break
                        else:
                            bytesProcessed += MultiPartSubMessageHeader.LENGTH
                            assert bytesProcessed <= len(innerMessage)
                            submessageStart = bytesProcessed
                            bytesProcessed += subMessageHeader.messageLength
                            if subMessageHeader.messageType in RELIABLE_MESSAGE_TYPES:
                                msgSeqId = seqIdCounter
                                seqIdCounter = NextSequenceId(seqIdCounter)
                            else:
                                msgSeqId = INVALID_RELIABLE_SEQ_ID
                            self.HandleSubMessage(innerMessage[submessageStart:bytesProcessed], subMessageHeader.messageType, \
                                                  msgSeqId, connectionInfo, sourceAddress)
                    assert bytesProcessed == len(innerMessage), (bytesProcessed, len(innerMessage))
                    assert (reliablePacketHeader.seqIdMax == INVALID_RELIABLE_SEQ_ID) or (seqIdCounter == NextSequenceId(reliablePacketHeader.seqIdMax)), (seqIdCounter, reliablePacketHeader)
                else:
                    assert reliablePacketHeader.seqIdMin == reliablePacketHeader.seqIdMax, (reliablePacketHeader.type, reliablePacketHeader.seqIdMin, reliablePacketHeader.seqIdMax)
                    self.HandleSubMessage(innerMessage, reliablePacketHeader.type, reliablePacketHeader.seqIdMin, \
                                          connectionInfo, sourceAddress)
        if not handledMessageType:
            sys.stderr.write("ReceiveData unknown/unhandled type of message!\r\n")
            # Pass it on anyway?
            if self.receiver:
                self.receiver.ReceiveData(buffer, sourceAddress)

    def Update(self):
        "Thread step function"
        self.ReceiveData()
        for key in list(self.connections.keys()):
            connectionInfo = self.connections[key]
            if connectionInfo.Update(self) is False:
                # Connection has timed out
                # Disconnect on timeout
                if self.receiver:
                    self.receiver.OnDisconnected(connectionInfo.address)
                del self.connections[key]
        while len(self.queue):
            self.lock.acquire()
            action, args = self.queue.pop(0)
            self.lock.release()
            action(*args)

    def Print(self):
        "Print some status information"
        return "ReliableTransport %d:\r\n%s" % (len(self.connections), '\r\n\t'.join([c.Print() for c in self.connections.values()]))

    def FindConnection(self, address, createIfNew):
        "Find a connection in the list or possibly create one"
        if address in self.connections.keys():
            return self.connections[address]
        elif createIfNew:
            newConnection = ReliableConnection(address)
            self.connections[address] = newConnection
            return newConnection
        else:
            return None
