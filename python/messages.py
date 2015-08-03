"""
THIS FILE IS A HACK! It should not be used beyond prototyping with the linux system on module.

This file re-defines cozmo message types as python classes. These definitions are hand built based on source code in
include/anki/cozmo/shared/MessageDefinitionsR2B.h If the messages used in that file are updated, the definitions here
will need to be updated as well.
"""
__author__  = "Daniel Casner"
__version__ = "918044192be58a4e128d4b0b6c1254ee726b080e" # Hash for the revision these python definitions are based on

import sys, struct

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
IE_RAW_GREY = 1
IE_RAW_RGB = 2
IE_YUYV  = 3
IE_BAYER = 4
IE_JPEG_GRAY  = 5
IE_JPEG_COLOR = 6
IE_JPEG_CHW = 7
IE_WEBP  = 8

CAMERA_RES_QUXGA = 0
CAMERA_RES_QXGA = 1
CAMERA_RES_UXGA = 2
CAMERA_RES_SXGA = 3
CAMERA_RES_XGA = 4
CAMERA_RES_SVGA = 5
CAMERA_RES_VGA = 6
CAMERA_RES_QSVGA = 6.5 # HACK! Not in the C++ code
CAMERA_RES_QVGA = 7
CAMERA_RES_QQVGA = 8
CAMERA_RES_QQQVGA = 9
CAMERA_RES_QQQQVGA = 10
CAMERA_RES_VERIFICATION_SNAPSHOT = 11
CAMERA_RES_COUNT = 12
CAMERA_RES_NONE = CAMERA_RES_COUNT

class MessageBase(struct.Struct):
    "Base class for messages implemented in Python"

    ID = 0
    FOMAT = []

    def __init__(self):
        "Initalize the structure definition from the class format field"
        struct.Struct.__init__(self, portableNamesToStructFormat(self.FORMAT))

    @classmethod
    def isa(cls, msg):
        "Returns true if the provided msg matches the type tag for this class. Argument can be bytes, string or int"
        argtype = type(msg)
        if argtype is int:
            return msg == cls.ID
        elif argtype is str: # Str has to come before bytes because they are the same in python 2
            return ord(msg[0]) == cls.ID
        elif argtype is bytes:
            return msg[0] == cls.ID
        else:
            raise ValueError("MessageBase doesn't know how to interprate a %s \"%s\"" % (argtype, msg))

    def serialize(self):
        "Convert python struct into C compatable binary struct"
        return struct.pack('b', self.ID) + self.pack(*self._getMembers())

    def deserialize(self, buffer):
        "Deserialize the received buffer"
        assert self.isa(buffer)
        self._setMembers(*self.unpack(buffer[1:]))


class ImageRequest(MessageBase):
    "ImageRequest message implementation for Python"

    ID = 20
    FORMAT = ["u8", # imageSendMode
              "u8", # Resolution
             ]

    def __init__(self, buffer=None, imageSendMode=ISM_OFF, resolution=CAMERA_RES_NONE):
        MessageBase.__init__(self)
        self.imageSendMode = imageSendMode
        self.resolution    = resolution
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

    IMAGE_CHUNK_SIZE = 1400
    ID = 63
    FORMAT = ["u32",  # imageId
              "u32",  # frameTimeStamp
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
        self.imageTimestamp = 0
        self.imageEncoding = IE_NONE
        self.imageChunkCount = 0
        self.chunkId = 0
        self.resolution = CAMERA_RES_NONE
        self.data = ""
        if buffer:
            self.deserialize(buffer)

    def _getMembers(self):
        "Convert the python class into a C struct compatible serial byte buffer"
        its = max(0, min(int(self.imageTimestamp), 4294967295))
        return self.imageId, its, len(self.data), self.imageEncoding, self.imageChunkCount, self.chunkId, self.resolution, self.data

    def _setMembers(self, *members):
        self.imageId, self.imageTimestamp, size, self.imageEncoding, self.imageChunkCount, self.chunkId, self.resolution, self.data = members
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

class RobotState(MessageBase):
    ID = 52
    FORMAT = [
        "u32", # Timestamp
        "u32", # pose frame id
        "f32", # pose X
        "f32", # pose Y
        "f32", # pose Z
        "f32", # pose angle
        "f32", # pose pitch angle
        "f32", # Left wheel speed in mm/s
        "f32", # Right wheel speed in mm/s
        "f32", # Head angle
        "f32", # Lift Angle
        "f32", # Lift Height
        "f32", # Raw Gyro Z
        "f32", # Raw accel Y
        "u32",  # Status, see RobotStatusFlag
        "u16", # last path ID
        "u8",  # Prox left
        "u8",  # Prox forward
        "u8",  # Prox right
        "s8",  # Current path segment, -1 if not traversing a path
        "u8",  # Num free segment slots
        "u8",  # Battery voltage x10
    ]

    class Pose(object):
        def __init__(self, x=0.0, y=0.0, z=0.0, angle=0.0, pitch=0.0):
            self.x = x
            self.y = y
            self.z = z
            self.angle = angle
            self.pitch = pitch

    def __init__(self, buffer=None):
        MessageBase.__init__(self)
        self.timestamp = 0
        self.poseFrameId = 0
        self.pose = self.Pose()
        self.leftWheelSpeed  = 0.0
        self.rightWheelSpeed = 0.0
        self.headAngle = 0.0
        self.liftAngle = 0.0
        self.liftHeight = 0.0
        self.rawGyroZ = 0.0
        self.rawAccelY = 0.0
        self.lastPathID = 0
        self.proxLeft = 255
        self.proxForward = 255
        self.proxRight = 255
        self.curPathSegment = -1
        self.numFreeSegmentSlots = 0
        self.battVolt10x = 0
        self.status = 0
        if buffer:
            self.deserialize(buffer)

    def _getMembers(self):
        return self.timestamp, self.poseFrameId, self.pose.x, self.pose.y, self.pose.z, self.pose.angle, \
               self.pose.pitch, self.leftWheelSpeed, self.rightWheelSpeed, self.headAngle, self.liftAngle, \
               self.liftHeight, self.rawGyroZ, self.rawAccelY, self.lastPathID, self.proxLeft, self.proxForward, \
               self.proxRight, self.curPathSegment, self.numFreeSegmentSlots, self.battVolt10x, self.status

    def _setMembers(self, *members):
        self.timestamp, self.poseFrameId, self.pose.x, self.pose.y, self.pose.z, self.pose.angle, \
        self.pose.pitch, self.leftWheelSpeed, self.rightWheelSpeed, self.headAngle, self.liftAngle, \
        self.liftHeight, self.rawGyroZ, self.rawAccelY, self.lastPathID, self.proxLeft, self.proxForward, \
        self.proxRight, self.curPathSegment, self.numFreeSegmentSlots, self.battVolt10x, self.status = members

    def __repr__(self):
        return "RobotState(timestamp=%d, ..., status=%d)" % (self.timestamp, self.status)

    @classmethod
    def unpackTimestamp(cls, buffer):
        return struct.unpack('I', buffer[1:5])(0)

class PrintText(MessageBase):
    ID = 62
    PRINT_TEXT_MSG_LENGTH = 50
    FORMAT = ['%ds' % (PRINT_TEXT_MSG_LENGTH)]

    def __init__(self, buffer=None, text=""):
        MessageBase.__init__(self)
        self.text = text
        if buffer:
            self.deserialize(buffer)

    def _getMembers(self):
        if sys.version_info.major < 3:
            return (self.text,)
        else:
            return (bytes(self.text, encoding="ASCII"),)

    def _setMembers(self, text):
        end = text.find(b"\x00")
        if end > -1:
            text = text[:end]
        if sys.version_info.major < 3:
            self.text = text
        else:
            self.text = text.decode("utf-8")

    def __repr__(self):
        return "PrintText(%s)" % self.text

    def __str__(self):
        return self.text

class DriveWheelsMessage(MessageBase):
    ID = 1
    FORMAT = ["f32", # Left wheel speed mmps
              "f32", # Right wheel speed mmps
             ]

    def __init__(self, *rest):
        "Create drive wheels message from either a buffer or left and right wheel speeds"
        MessageBase.__init__(self)
        self.lws = 0.0
        self.rws = 0.0
        if len(rest) == 1: # Deserialize from buffer
            self.deserialize(rest[0])
        elif len(rest) == 2:
            self._setMembers(*rest)

    def _getMembers(self):
        return self.lws, self.rws

    def _setMembers(self, *members):
        self.lws, self.rws = members

    def __repr__(self):
        return "DriveWheelsMessage(%f, %f)" % self._getMembers()

class MoveLiftMessage(MessageBase):
    ID = 3
    FORMAT = ["f32", # speed_rad_per_sec
             ]

    def __init__(self, buffer=None, speed=0.0):
        "Create move lift message"
        MessageBase.__init__(self)
        self.speed = speed
        if buffer is not None:
            self.deserialize(buffer)

    def _getMembers(self):
        return self.speed,

    def _setMembers(self, speed):
        self.speed = speed

    def __repr__(self):
        return "MoveLiftMessage(%f)" % self.speed

class MoveHeadMessage(MessageBase):
    ID = 4
    FORMAT = ["f32", # speed_rad_per_sec
             ]

    def __init__(self, buffer=None, speed=0.0):
        "Create move lift message"
        MessageBase.__init__(self)
        self.speed = speed
        if buffer is not None:
            self.deserialize(buffer)

    def _getMembers(self):
        return self.speed,

    def _setMembers(self, speed):
        self.speed = speed

    def __repr__(self):
        return "MoveHeadMessage(%f)" % self.speed

class SetLiftHeightMessage(MessageBase):
    ID = 5
    FORMAT = ["f32", # height mm,
              "f32", # max speed rad/sec
              "f32", # accel rad/sec^2
             ]

    def __init__(self, *rest):
        MessageBase.__init__(self)
        self.height = 0.0
        self.max_speed = 1.0
        self.accel = 1.0
        if len(rest) > 0:
            if type(rest[0]) in (float, int):
                self.height = rest[0]
            else:
                self.deserialize(rest[0])
                return
        if len(rest) > 1:
            self.max_speed = rest[1]
        if len(rest) > 2:
            self.accel = rest[2]

    def _getMembers(self):
        return self.height, self.max_speed, self.accel

    def _setMembers(self, *members):
        self.height, self.max_speed, self.accel = members

    def __repr__(self):
        return "SetLiftHeightMessage(height=%f, max_speed=%f, accel=%f)" % self._getMembers()

class SetHeadAngleMessage(MessageBase):
    ID = 6
    FORMAT = ["f32", # angle radians,
              "f32", # max speed rad/sec
              "f32", # accel rad/sec^2
             ]

    def __init__(self, *rest):
        MessageBase.__init__(self)
        self.angle = 0.0
        self.max_speed = 1.0
        self.accel = 1.0
        if len(rest) > 0:
            if type(rest[0]) in (float, int):
                self.angle = rest[0]
            else:
                self.deserialize(rest[0])
                return
        if len(rest) > 1:
            self.max_speed = rest[1]
        if len(rest) > 2:
            self.accel = rest[2]

    def _getMembers(self):
        return self.angle, self.max_speed, self.accel

    def _setMembers(self, *members):
        self.angle, self.max_speed, self.accel = members

    def __repr__(self):
        return "SetHeadAngleMessage(angle=%f, max_speed=%f, accel=%f)" % self._getMembers()

class StopAllMotorsMessage(MessageBase):
    ID = 7
    FORMAT = []

    def _getMembers(self):
        return tuple()

    def _setMembers(self, *members):
        pass

    def __repr__(self):
        return "StopAllMotorsMessage"


class StartTestMode(MessageBase):
    """Start a test with given parameters"""
    ID = 21
    FORMAT = ["s32",
              "s32",
              "s32",
              "u8" # Mode
              ]

    def __init__(self, mode=0, parameters=[0,0,0], buffer=None):
        MessageBase.__init__(self)
        self.mode = mode
        self.p = parameters
        if buffer:
            self.deserialize(buffer)

    def _getMembers(self):
        return self.p + [self.mode]

    def _setMembers(self, *members):
        self.p = members[:3]
        self.mode = members[3]

    def __repr__(self):
        return "StartTestMode(mode=%d parameters=%s)" % (self.mode, repr(self.p))


class FlashBlockIDs(MessageBase):
    """Instruct reach block to visually indicate it's ID"""
    ID = 40
    FORMAT = []

    def _getMembers(self):
        return tuple()

    def _setMembers(self, *members):
        pass

    def __repr__(self):
        return "FlashBlockIDs"


class SetBlockLights(MessageBase):
    """Instruct robot to instruct block to set lights to colors"""
    NUM_LIGHTS = 4
    ID = 39
    FORMAT = [("%dI" % NUM_LIGHTS), # on color
              ("%dI" % NUM_LIGHTS), # off color
              ("%dI" % NUM_LIGHTS), # on period ms
              ("%dI" % NUM_LIGHTS), # off period ms
              ("%dI" % NUM_LIGHTS), # on transition ms
              ("%dI" % NUM_LIGHTS), # off transition ms
              "u8",  # blockID
             ]

    def __init__(self, buffer=None, blockID=0, lights=None):
        MessageBase.__init__(self)
        self.blockID = blockID
        self.onColor       = [0] * self.NUM_LIGHTS
        self.offColor      = [0] * self.NUM_LIGHTS
        self.onPeriod      = [0] * self.NUM_LIGHTS
        self.offPeriod     = [0] * self.NUM_LIGHTS
        self.onTransition  = [0] * self.NUM_LIGHTS
        self.offTransition = [0] * self.NUM_LIGHTS
        if lights is not None:
            if len(lights) != self.NUM_LIGHTS:
                raise ValueError("SetBlockLights, lights argument must have exactly %d elements, given %d" % (self.NUM_LIGHTS, len(lights)))
            else:

                for i, l in enumerate(lights):
                    if len(l) != 6:
                        raise ValueError("SetBlockLights each light must be a 6-tuple, argument %d given %s" % (i, repr(l)))
                    else:
                        self.onColor[i], self.offColor[i], self.onPeriod[i], self.offPeriod[i], self.onTransition[i], self.offTransition[i] = l
        if buffer is not None:
            self.deserialize(buffer)

    def __repr__(self):
        return "SetBlockLights(blockID=%d, ...)" % (self.blockID)

    def _getMembers(self):
        return self.onColor + self.offColor + self.onPeriod + self.offPeriod + self.onTransition + self.offTransition + [self.blockID]

    def _setMembers(self, *members):
        self.onColor       = list(members[self.NUM_LIGHTS*0:self.NUM_LIGHTS*1])
        self.offColor      = list(members[self.NUM_LIGHTS*1:self.NUM_LIGHTS*2])
        self.onPeriod      = list(members[self.NUM_LIGHTS*2:self.NUM_LIGHTS*3])
        self.offPeriod     = list(members[self.NUM_LIGHTS*3:self.NUM_LIGHTS*4])
        self.onTransition  = list(members[self.NUM_LIGHTS*4:self.NUM_LIGHTS*5])
        self.offTransition = list(members[self.NUM_LIGHTS*5:self.NUM_LIGHTS*6])
        self.blockID = members[self.NUM_LIGHTS]
