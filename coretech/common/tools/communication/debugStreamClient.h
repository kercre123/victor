/**
File: debugStreamClient.h
Author: Peter Barnum
Created: 2013

Simple class to handle tcp connections to process messages made of SerializedBuffer objects

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _DEBUG_STREAM_CLIENT_H_
#define _DEBUG_STREAM_CLIENT_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/serialize.h"

#include "communication.h"
#include "threadSafeQueue.h"

#include <string>

#ifdef _MSC_VER

#include <tchar.h>
#include <strsafe.h>

#else // #ifdef _MSC_VER

#include <unistd.h>
#include <pthread.h>

#endif // #ifdef _MSC_VER ... #else

namespace Anki
{
  namespace Embedded
  {
#ifdef _MSC_VER
#define ThreadResult DWORD WINAPI
#else
#define ThreadResult void*
#endif

    class DebugStreamClient
    {
    public:

      class Object
      {
      public:
        char typeName[Anki::Embedded::SerializedBuffer::DESCRIPTION_STRING_LENGTH];
        char objectName[Anki::Embedded::SerializedBuffer::DESCRIPTION_STRING_LENGTH];

        void * buffer; //< This buffer is allocated by DebugStreamClient, and the user must free it
        s32 bufferLength; //< The number of bytes allocated for buffer

        void * startOfPayload; //< This points after any header in buffer. Just cast this pointer to the appropriate type.

        f64 timeReceived; //< All objects from the same message will have the same timeReceived

        Object();

        bool IsValid() const;
      };

      // Example: DebugStreamClient("192.168.3.30", 5551)
      DebugStreamClient(const char * ipAddress, const s32 port);

      // Close the socket and stop the parsing threads
      Anki::Result Close();

      // Blocks until an Object is available, the returns it. To use an object, just cast its buffer based on its typeName and objectName.
      //
      // If the object bufferLength is zero, it's not a big issue, just ignore the object
      // If the object bufferLength is less than zero, something failed
      Object GetNextObject();

      bool get_isRunning() const;

    protected:

      typedef struct {
        u8 * data;
        u8 * rawDataPointer; // Unaligned
        s32 curDataLength;
        s32 maxDataLength;
        f64 timeReceived; // The time (in seconds) when the receiving of this buffer was complete
      } RawBuffer;

      static const s32 USB_BUFFER_SIZE = 5000;
      static const s32 MESSAGE_BUFFER_SIZE = 1000000;

      const char * ipAddress;
      s32 port;

      volatile bool isRunning; //< If true, keep working. If false, close everything down
      volatile bool isConnectionThreadActive;
      volatile bool isParseBufferThreadActive;

      ThreadSafeQueue<RawBuffer> rawMessageQueue;
      ThreadSafeQueue<DebugStreamClient::Object> parsedObjectQueue;

      // Allocates using malloc
      static RawBuffer AllocateNewRawBuffer(const s32 bufferRawSize);

      // Process the buffer, and adds any parsed objects to parsedObjectQueue. Frees the buffer on completion.
      static void ProcessRawBuffer(RawBuffer &buffer, ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue, const bool requireMatchingSegmentLengths);

      // Grab data from the socket and put in a big temporary buffer, as fast as possible
      static ThreadResult ConnectionThread(void *threadParameter);

      // Once the big temporary buffer is filled, parse it
      static ThreadResult ParseBufferThread(void *threadParameter);
    }; // DebugStreamClient
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _DEBUG_STREAM_CLIENT_H_