/**
File: messageHandling.h
Author: Peter Barnum
Created: 2013

Simple USB routines to process messages

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

enum BufferAction {
  BUFFER_ACTION_SAVE,
  BUFFER_ACTION_DISPLAY
};

#define PRINTF_ALL_RECEIVED

#define BIG_BUFFER_SIZE 100000000

const bool swapEndianForHeaders = true;
const bool swapEndianForContents = true;

// Search rawBuffer for the 8-byte serialized buffer headers and footers.
// startIndex is the location after the header, or -1 if one is not found
// endIndex is the location before the footer, or -1 if one is not found
void FindSerializedBuffer(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex);

// Process the buffer using the BufferAction action. Optionally, free the buffer on completion.
void ProcessRawBuffer(RawBuffer &buffer, const std::string outputFilenamePattern, const bool freeBuffer, const BufferAction action);

#endif // #ifndef _MESSAGE_HANDLING_H_