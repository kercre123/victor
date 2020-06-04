#ifndef _CF_PACKET_H
#define _CF_PACKET_H
//============================================================================
// 635 Linux Demonstration Code.
// 635_packet,h: Ultra-simple 635 command-line communications example
// Copyright 2005, Crystalfontz America, Inc. Written by Brent A. Crosby
// www.crystalfontz.com, brent@crystalfontz.com
//============================================================================
#define MAX_DATA_LENGTH 22
//============================================================================
#define MAX_COMMAND 35
//============================================================================
typedef struct
  {
  ubyte
    command;
  ubyte
    data_length;
  ubyte
    data[MAX_DATA_LENGTH];
  CF_WORD_UNION
    CRC;
  }COMMAND_PACKET;
//============================================================================
word get_crc(ubyte *bufptr,word len,word seed);
//============================================================================
extern COMMAND_PACKET
  incoming_command;
extern COMMAND_PACKET
  outgoing_response;
//============================================================================
ubyte check_for_packet(void);
void send_packet(void);
//============================================================================
#endif
