"""
THIS FILE IS A HACK! It should not be used beyond prototyping with the linux system on module.

This file re-defines cozmo message types as python classes. These definitions are hand built based on source code in
include/anki/cozmo/shared/MessageDefinitionsR2B.h If the messages used in that file are updated, the definitions here
will need to be updated as well.
"""
__author__  = "Daniel Casner"
__version__ = "29210a1860673f109936598fd58cf6757c52e719" # Hash for the revision these python definitions are based on

import struct

def portableNamesToStructFormat(names):
    table = {
        "u8" : "B",
        "u16": "H",
        "u32": "I",
        "u64": "Q",
        "s8" : "b",
        "s16": "h",
        "s32": "i",
        "s64": "q",
        "f32": "f",
        "f64": "d",
    }
    return "".join([table.get(n, n) for n in names])

ISM_OFF = 0
ISM_STREAM = 1
ISM_SINGLE_SHOT = 2
IE_NONE  = 0
IE_YUYV  = 1
IE_BAYER = 2
IE_JPEG  = 3
IE_WEBP  = 4
CAMERA_RES_QUXGA = 0
CAMERA_RES_QXGA = 1
CAMERA_RES_UXGA = 2
CAMERA_RES_XGA = 3
CAMERA_RES_SVGA = 4
CAMERA_RES_VGA = 5
CAMERA_RES_QVGA = 6
CAMERA_RES_QQVGA = 7
CAMERA_RES_QQQVGA = 8
CAMERA_RES_QQQQVGA = 9
CAMERA_RES_VERIFICATION_SNAPSHOT = 10
CAMERA_RES_COUNT = 11
CAMERA_RES_NONE = CAMERA_RES_COUNT

class MessageBase(struct.Struct):
    "Base class for messages implemented in Python"

    ID = 0
    FOMAT = []

    def __init__(self):
        "Initalize the structure definition from the class format field"
        struct.Struct.__init__(self, portableNamesToStructFormat(self.FORMAT))

    def serialize(self):
        "Convert python struct into C compatable binary struct"
        return chr(self.ID) + self.pack(*self._getMembers())

    def deserialize(self, buffer):
        "Deserialize the received buffer"
        assert ord(buffer[0]) == self.ID, ("Wrong message ID: %d, expected %d" % (ord(buffer[0]), self.ID))
        self._setMembers(*self.unpack(buffer[1:]))


class ImageRequest(MessageBase):
    "ImageRequest message implementation for Python"

    ID = 20
    FORMAT = ["u8", # imageSendMode
              "u8", # Resolution
             ]

    def __init__(self, buffer=None):
        MessageBase.__init__(self)
        self.imageSendMode = ISM_OFF
        self.resolution = CAMERA_RES_NONE
        if buffer:
            self.deserialize(buffer)

    def _getMembers(self):
        return self.imageSendMode, self.resolution

    def _setMembers(self, ism, res):
        self.imageSendMode = ism
        self.resolution    = res

    def __repr__(self):
        return "ImageRequest(%d, %d)" % (self.imageSendMode, self.resolution)

class ImageChunk(MessageBase):
    "ImageChunk message implementation for Python"

    IMAGE_CHUNK_SIZE = 1024
    ID = 66
    FORMAT = ["u32",  # imageId
              "u16",  # chunkSize
              "u8",   # imageEncoding
              "u8",   # imageChunkCount
              "u8",   # chunkId
              "u8",   # resolution
              ("%ds" % (IMAGE_CHUNK_SIZE)) # data
              ]

    def __init__(self, buffer=None):
        "Initalizes the base class with the specified member type definitions"
        MessageBase.__init__(self)
        self.imageId = 0
        self.imageEncoding = IE_NONE
        self.imageChunkCount = 0
        self.chunkId = 0
        self.resolution = CAMERA_RES_NONE
        self.data = ""
        if buffer:
            self.deserialize(buffer)

    def _getMembers(self):
        "Convert the python class into a C struct compatible serial byte buffer"
        return self.imageId, len(self.data), self.imageEncoding, self.imageChunkCount, self.chunkId, self.resolution, self.data

    def _setMembers(self, *members):
        self.imageId, size, self.imageEncoding, self.imageChunkCount, self.chunkId, self.resolution, self.data = members
        assert size <= self.IMAGE_CHUNK_SIZE, ("Size, %d, too large" % size)
        self.data = self.data[:size]

    def takeChunk(self, buffer):
        """Grabs up to IMAGE_CHUNK_SIZE bytes from buffer sets them as data.
        Returns the remainder of the buffer or an empty string if everything was consumed."""
        if len(buffer) > self.IMAGE_CHUNK_SIZE:
            self.data = buffer[:self.IMAGE_CHUNK_SIZE]
            return buffer[self.IMAGE_CHUNK_SIZE:]
        else:
            self.data = buffer
            return ""

    def __repr__(self):
        if len(self.data) > 8:
            dataRepr = self.data[:5] + '...'
        else:
            dataRepr = self.data
        return "ImageChunk(imageId=%d, imageEncoding=%d, chunkId=%d, resolution=%d, data[%d]=%s)" % (self.imageId, self.imageEncoding, self.chunkId, self.resolution, len(self.data), dataRepr)

class PingMessage(MessageBase):
    ID = 56
    FORMAT = []

    def _getMembers(self):
        return tuple()

    def _setMembers(self, *members):
        pass

    def __repr__(self):
        return "PingMessage"

class ClientConnectionStatus(MessageBase):
    """Struct for SoM Radio state information.
    This message is not intendent to be used beyond the SoM prototype."""

    ID = 58
    FORMAT = ["u8", # wifi state
              "u8", # bluetooth state
              ]

    def __init__(self, wifi=0, bluetooth=0, buffer=None):
        MessageBase.__init__(self)
        self.wifiState = wifi
        self.blueState = bluetooth
        if buffer:
            self.deserialize(buffer)

    def _getMembers(self):
        return self.wifiState, self.blueState

    def _setMembers(self, wifi, blue):
        self.wifiState = wifi
        self.blueState = blue

    def __repr__(self):
        return "SoMRadioState(wifi=%d, bluetooth=%d)" % self._getMembers
