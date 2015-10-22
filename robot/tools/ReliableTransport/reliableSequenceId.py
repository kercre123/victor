"""
Python module for dealing with sequence IDs since they are imported everywhere
"""
__author__ = "Daniel Casner <daniel@anki.com>"

import struct

INVALID_RELIABLE_SEQ_ID = 0
MIN_RELIABLE_SEQ_ID     = 1
MAX_RELIABLE_SEQ_ID     = 65534

def PreviousSequenceId(inSeqId):
    "Returns the previous valid sequence id for a given sequence id"
    assert ((inSeqId >= MIN_RELIABLE_SEQ_ID) and (inSeqId <= MAX_RELIABLE_SEQ_ID))
    if inSeqId == MIN_RELIABLE_SEQ_ID:
        return MAX_RELIABLE_SEQ_ID
    else:
        return inSeqId - 1

def NextSequenceId(inSeqId):
    "Returns the next valid sequence id for a given sequence id"
    assert ((inSeqId >= MIN_RELIABLE_SEQ_ID) and (inSeqId <= MAX_RELIABLE_SEQ_ID))
    if inSeqId == MAX_RELIABLE_SEQ_ID:
        return MIN_RELIABLE_SEQ_ID
    else:
        return inSeqId + 1

def IsSequenceIdInRange(seqId, minSeqId, maxSeqId):
    "Return whether a sequence ID is between two other sequence IDs accounting for wrapping"
    if (maxSeqId >= minSeqId): # Not looped
        return ((seqId >= minSeqId) and (seqId <= maxSeqId))
    else: # IDs have looped arround
        return ((seqId >= minSeqId) or  (seqId <= maxSeqId))

class EReliableMessageType:
    "Enum for reliable message types"
    Invalid = 0
    ConnectionRequest = 1
    ConnectionResponse = 2
    DisconnectRequest = 3
    SingleReliableMessage = 4
    SingleUnreliableMessage = 5
    MultiPartMessage = 6 # Big message in n pieces
    MultipleReliableMessages = 7 # several reliable messages grouped grouped into 1 packet (increase chance of acking all together)
    MultipleUnreliableMessages = 8 # several unreliable messages grouped into 1 packet (reduce number of packets sent)
    MultipleMixedMessages = 9 # several (reliable and unreliable) messages grouped into 1 packet (combo of above)
    ACK = 10
    Ping = 11
    Count = 12

RELIABLE_MESSAGE_TYPES = [
    EReliableMessageType.ConnectionRequest,
    EReliableMessageType.ConnectionResponse,
    EReliableMessageType.DisconnectRequest,
    EReliableMessageType.SingleReliableMessage,
    EReliableMessageType.MultiPartMessage
]
UNRELIABLE_MESSAGE_TYPES = [
    EReliableMessageType.SingleUnreliableMessage,
    EReliableMessageType.ACK,
    EReliableMessageType.Ping,
]

class MultiPartSubMessageHeader(struct.Struct):
    "Header inside a packet payload between messages"

    LENGTH = 3 # How long it's supposed to be for protocol

    def __init__(self, messageType=EReliableMessageType.Invalid, messageLength=0, buffer=None, offset=0):
        struct.Struct.__init__(self, "<BH")
        self.messageType = messageType
        self.messageLength = messageLength
        if buffer is not None:
            self._deserialize(buffer, offset)

    def serialize(self, buffer=None, offset=0):
        "Serialize the header for the wire"
        values = self.messageType, self.messageLength
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
        self.messageType, self.messageLength = self.unpack_from(buffer, offset)

    def __repr__(self):
        return "MultiPartSubMessageHeader(%d, %d)" % (self.messageType, self.messageLength)
