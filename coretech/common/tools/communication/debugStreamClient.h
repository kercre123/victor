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
#include "anki/tools/threads/threadSafeQueue.h"

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
    typedef HANDLE ThreadHandle;
#define ThreadResult DWORD WINAPI
#else
    typedef pthread_t ThreadHandle;
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

      class ObjectToSave : public DebugStreamClient::Object
      {
      public:
        static const s32 SAVE_FILENAME_PATTERN_LENGTH = 256;
        char filename[SAVE_FILENAME_PATTERN_LENGTH];

        ObjectToSave();

        ObjectToSave(DebugStreamClient::Object &object, const char * filename);
      };

      // Connect via TCP
      // Example: DebugStreamClient("192.168.3.30", 5551)
      DebugStreamClient(const char * ipAddress, const s32 port);

      // Connect via UART serial
      // Example: DebugStreamClient(11, 1000000)
      DebugStreamClient(const s32 comPort, const s32 baudRate, const char parity = 'N', const s32 dataBits = 8, const s32 stopBits = 1);

      // Warning: calling the destructor from a different thread than the constructor is a race condition
      ~DebugStreamClient();

      // Close the socket and stop the parsing threads
      // Warning: calling Close from a different thread than the constructor is a race condition
      Result Close();

      // Blocks until an Object is available, the returns it. To use an object, just cast its buffer based on its typeName and objectName.
      //
      // If the object bufferLength is zero, it's not a big issue, just ignore the object
      // If the object bufferLength is less than zero, something failed
      Object GetNextObject(s32 maxAttempts = s32_MAX);

      // Doesn't block, as the saving is done by a separate, low-priority thread
      // It frees the object memory after the save is complete
      Result SaveObject(Object &object, const char * filename);

      // Returns if the object's threads are acquiring and processing new data
      // This is false while the object is starting up and closing down.
      bool get_isRunning() const;

    protected:

      typedef struct {
        u8 * data;
        u8 * rawDataPointer; // Unaligned
        s32 curDataLength;
        s32 maxDataLength;
        f64 timeReceived; // The time (in seconds) when the receiving of this buffer was complete
      } RawBuffer;

      static const s32 CONNECTION_BUFFER_SIZE = 5000;
      static const s32 MESSAGE_BUFFER_SIZE = 1000000;

      char saveFilenamePattern[DebugStreamClient::ObjectToSave::SAVE_FILENAME_PATTERN_LENGTH];

      bool isValid;

      bool isSocket; //< Either Socket of Serial

      // If Socket
      const char * socket_ipAddress;
      s32 socket_port;

      // If Serial
      s32 serial_comPort;
      s32 serial_baudRate;
      char serial_parity;
      s32 serial_dataBits;
      s32 serial_stopBits;

      volatile bool isRunning; //< If true, keep working. If false, close everything down
      //volatile bool isConnectionThreadActive;
      //volatile bool isParseBufferThreadActive;
      //volatile bool isSaveObjectThreadActive;

      ThreadHandle connectionThread;
      ThreadHandle parseBufferThread;
      ThreadHandle saveObjectThread;

      ThreadSafeQueue<DebugStreamClient::RawBuffer> rawMessageQueue;
      ThreadSafeQueue<DebugStreamClient::Object> parsedObjectQueue;
      ThreadSafeQueue<DebugStreamClient::ObjectToSave> saveObjectQueue;

      // Initialize the queues and spawn the threads
      Result Initialize();

      // Allocates using malloc
      static DebugStreamClient::RawBuffer AllocateNewRawBuffer(const s32 bufferRawSize);

      // Process the buffer, and adds any parsed objects to parsedObjectQueue. Frees the buffer on completion.
      static void ProcessRawBuffer(DebugStreamClient::RawBuffer &buffer, ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue, const bool requireMatchingSegmentLengths);

      // Grab data from the socket and put in a big temporary buffer, as fast as possible
      static ThreadResult ConnectionThread(void *threadParameter);

      // Once the big temporary buffer is filled, parse it
      static ThreadResult ParseBufferThread(void *threadParameter);

      // Save any files that are put in the saveObjectQueue
      static ThreadResult SaveObjectThread(void *threadParameter);

    private:
      // Do not use these
      DebugStreamClient(const DebugStreamClient& in);
      DebugStreamClient& operator= (const DebugStreamClient& in);
    }; // DebugStreamClient
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _DEBUG_STREAM_CLIENT_H_