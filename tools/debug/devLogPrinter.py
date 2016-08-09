#!/usr/bin/python

import sys
import struct

def main(filename, types):
    with open(filename, 'rb') as f:
        while readMessage(f, types):
            pass

# take byte chunk and format it into printed hex bytes
def getDataString(chunk):
    return ' '.join(map((lambda x: "%0.2x" % ord(x)), chunk))

# read one message, return true if successful & should keep reading
def readMessage(f, types):
    size = f.read(4)
    if (len(size) == 0):
        # eof
        return False

    timestamp = f.read(4)
    if (len(size) != 4 or len(timestamp) != 4):
        print "bad size of size/time"
        return False

    # message format is: 4 byte size, 4 byte timestamp, message data
    # message size is little endian
    size = struct.unpack('<1I', size)[0]
    if (size <= 8):
        print "invalid size %d for message" % size
        return False

    # read remaining message data
    size = size - 8
    data = f.read(size)
    if (len(data) != size):
        print "read %d but wanted %d" % (len(data), size)
        return False

    # first byte is the type (tag) of CLAD message, rest is the message data
    msgType = ord(data[0])
    data = data[1:]

    # check if this message type was filtered out and we should skip printing it
    if (len(types) != 0 and ('%0.2x' % msgType) not in types):
        return True

    # split data into groups of 16 for pretty formatting
    dataChunks = [data[i:i+16] for i in range(0,len(data),16)]

    # print message type and first line of data, then subsequent lines if there's >1 chunk
    print 'type %0.2x data %s' % (msgType, getDataString(dataChunks[0]) if len(dataChunks) > 0 else '')
    for chunk in dataChunks[1:]:
        #      type 00 data
        print '             %s' % getDataString(chunk)

    return True


if __name__ == '__main__':
    if (len(sys.argv) < 2):
        print 'usage: %s filename [types]' % sys.argv[0]
        print '  [types] is optional list of message types, only these will be printed'
    else:
        main(sys.argv[1], sys.argv[2:])
