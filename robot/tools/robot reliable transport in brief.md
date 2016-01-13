# Robot Reliable Transport in Brief

Not that this describes the format of packets coming from the robot. The embedded implementation only uses a subset of the general reliable transport protocol so packets from other sources may be more complex.

## Packet format

### Header
Each packet will start with

1. The 7 byte preamble `COZ\x03RE\x01`
2. 1 byte enum indicating the reliable message type. For the robot this is always `eRMT_MultipleMixedMessages`
3. 2 bytes, 16 bit number, indicating the lowest sequence ID of the reliable messages in this packet
4. 2 bytes, 16 bit number, indicating the highest sequence ID of the reliable messages in this packet
5. 2 bytes, 16 bit number, indicating the sequence number of the latest reliable message received and acknowledged by the robot.

All told the header is 14 bytes.

### Sub message headers
After the header, the packet will contain *0* or more messages, each prefixed with a 3 byte header which contains:

1. 1 byte indicating the message type. Application messages from the robot will either be `eRMT_SingleReliableMessage` or `eRMT_SingleUnreliableMessage`; anything else is a reliable transport control message.
2. 2 bytes, 16 bit number, indicating the length of the message, not including this 3 byte message header.

### Robot messages
The first byte in a robot message will always be the message tag. For ImageChunks, this will always be `0x82`. The tag is part of the length in the sub message header.

## Parsing image Messages
To simply collect image Messages

```
Skip the first 14 bytes of the packet.
While not at end of packet:
    Read the 3 byte sub message header to get the length of the message
    If 1 byte tag after header is image chunk:
        parse the image chunk
    Advance past the message + header
```
