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
#include "anki/common/robot/serialize.h"

#include "communication.h"
#include "threadSafeQueue.h"

#include <string>

#ifdef _MSC_VER
#define ThreadResult DWORD WINAPI
#else
#define ThreadResult void*
#endif

class DebugStreamParserThread
{
public:

  class Object
  {
  public:
    char typeName[Anki::Embedded::SerializedBuffer::DESCRIPTION_STRING_LENGTH];
    char objectName[Anki::Embedded::SerializedBuffer::DESCRIPTION_STRING_LENGTH];

    void * buffer; //< This buffer is allocated by DebugStreamParserThread, and the user must free it
    s32 bufferLength;

    const void * startOfPayload; //< This points after any header in buffer. Just cast this pointer to the appropriate type.

    f32 timeReceived; //< All objects from the same message will have the same timeReceived

    Object();

    // Allocate the memory for this object
    Object(const s32 bufferLength);

    bool IsValid() const;
  };

  // Example: DebugStreamParserThread("192.168.3.30", 5551)
  DebugStreamParserThread(const char * ipAddress, const s32 port);

  // Close the socket and stop the parsing thread
  Anki::Result Close();

  // Blocks until an Object is available, the returns it. To use an object, just cast its buffer based on its typeName and objectName.
  //
  // If the object bufferLength is zero, it's not a big issue, just ignore the object
  // If the object bufferLength is less than zero, something failed
  Object GetNextObject();

  bool get_isConnected() const;

protected:

  typedef struct {
    u8 * data;
    u8 * rawDataPointer; // Unaligned
    s32 curDataLength;
    s32 maxDataLength;
  } DisplayRawBuffer;

  //typedef struct {
  //  u8 * data;
  //  s32 dataLength;
  //} RawBuffer;

  static const s32 USB_BUFFER_SIZE = 100000;
  static const s32 MESSAGE_BUFFER_SIZE = 1000000;

  Socket socket;
  bool isConnected;

  ThreadSafeQueue<DisplayRawBuffer> rawMessageQueue;
  ThreadSafeQueue<DebugStreamParserThread::Object> parsedObjectQueue;

  static DisplayRawBuffer AllocateNewRawBuffer(const s32 bufferRawSize);

  // Process the buffer using the BufferAction action. Optionally, free the buffer on completion.
  static void ProcessRawBuffer(DisplayRawBuffer &buffer, ThreadSafeQueue<DebugStreamParserThread::Object> &parsedObjectQueue, const bool requireMatchingSegmentLengths);

  // Grab data from the socket and put in a big temporary buffer, as fast as possible
  static ThreadResult ConnectionThread(void *threadParameter);

  // Once the big temporary buffer is filled, parse it
  static ThreadResult ParseBufferThread(void *threadParameter);
}; // DebugStreamParserThread

#endif // #ifndef _MESSAGE_HANDLING_H_