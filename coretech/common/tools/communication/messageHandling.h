/**
File: messageHandling.h
Author: Peter Barnum
Created: 2013

Simple routines to process messages made of SerializedBuffer objects

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _MESSAGE_HANDLING_H_
#define _MESSAGE_HANDLING_H_

#include "anki/common/robot/config.h"

#include <string>

typedef struct {
  u8 * data;
  s32 dataLength;
} RawBuffer;

typedef struct {
  u8 * data;
  u8 * rawDataPointer; // Unaligned
  s32 curDataLength;
  s32 maxDataLength;
} DisplayRawBuffer;

//#define PRINTF_ALL_RECEIVED

#define BIG_BUFFER_SIZE 100000000

// Process the buffer using the BufferAction action. Optionally, free the buffer on completion.
void ProcessRawBuffer_Save(RawBuffer &buffer, const std::string outputFilenamePattern, const bool freeBuffer, const bool requireCRCmatch);
void ProcessRawBuffer_Display(DisplayRawBuffer &buffer, const bool requireCRCmatch);

#endif // #ifndef _MESSAGE_HANDLING_H_