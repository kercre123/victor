"""
THIS FILE IS A HACK! It should not be used beyond prototyping with the linux system on module.

This file re-defines cozmo message types as python classes. These definitions are hand built based on source code in
include/anki/cozmo/shared/MessageDefinitionsR2B.h If the messages used in that file are updated, the definitions here
will need to be updated as well.
"""
__author__  = "Daniel Casner"
__version__ = "a7fdcb163a07a9a1ac59184e08106d60d004ce5e" # Hash for the revision these python definitions are based on

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

IE_NONE  = 0
IE_YUYV  = 1
IE_BAYER = 2
IE_JPEG  = 3
CAMERA_RES_VGA = 0
CAMERA_RES_QVGA = 1
CAMERA_RES_QQVGA = 2
CAMERA_RES_QQQVGA = 3
CAMERA_RES_QQQQVGA = 4
CAMERA_RES_VERIFICATION_SNAPSHOT = 5
CAMERA_RES_COUNT = 6
CAMERA_RES_NONE = CAMERA_RES_COUNT


class ImageRequest(struct.Struct):
    "ImageRequest message implementation for Python"

    ID = 19
    FORMAT = ["u8", # ID
              "u8", # imageSendMode
              "u8", # Resolution
             ]

    def __init__(self, buffer=None):
        struct.Struct.__init__(self, portableNamesToStructFormat(self.FORMAT))
        self.imageSendMode = IE_NONE
        self.resolution = CAMERA_RES_NONE
        if buffer:
            self.deserialize(buffer)

    def serialize(self):
        "Convert python struct into C compatable binary struct"
        return self.pack(self.ID, self.imageSendMode, self.resolution)

    def deserialize(self, buffer):
        "Deserialize received buffer"
        msgId, self.imageSendMode, self.resolution = self.unpack(buffer)
        assert(msgId == self.ID)

    def __repr__(self):
        return "ImageRequest(%d, %d)" % (self.imageSendMode, self.resolution)

class ImageChunk(struct.Struct):
    "ImageChunk message implementation for Python"

    IMAGE_CHUNK_SIZE = 1024
    ID = 41
    FORMAT = ["u8",   # ID
              "u16",  # chunkSize
              "u8",   # imageId
              "u8",   # imageEncoding
              "u8",   # chunkId
              "u8",   # resolution
              ("%ds" % (IMAGE_CHUNK_SIZE)) # data
              ]

    def __init__(self, buffer=None):
        "Initalizes the base class with the specified member type definitions"
        struct.Struct.__init__(self, portableNamesToStructFormat(self.FORMAT))
        self.imageId = 0
        self.imageEncoding = IE_NONE
        self.chunkId = 0
        self.resolution = CAMERA_RES_NONE
        self.data = ""
        if buffer:
            self.deserialize(buffer)

    def serialize(self):
        "Convert the python class into a C struct compatible serial byte buffer"
        return self.pack(self.ID, len(self.data), self.imageId, self.imageEncoding, self.chunkId, self.resolution, self.data)

    def deserialize(self, buffer):
        "Deserialize a received message"
        msgId, size, self.imageId, self.imageEncoding, self.chunkId, self.resolution, self.data = self.unpack(buffer)
        assert(msgId == self.ID)
        assert(size <= self.IMAGE_CHUNK_SIZE)
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
