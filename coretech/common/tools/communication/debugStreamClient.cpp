/**
File: debugStreamClient.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "debugStreamClient.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"
#include "anki/common/robot/fixedLengthList.h"

#include "anki/vision/robot/fiducialMarkers.h"
#include "anki/vision/robot/binaryTracker.h"

#include <ctime>
#include <vector>

#undef printf
#define printf (Do not use printf on this thread)

using namespace std;

static const s32 scratchSize = 1000000;
static u8 scratchBuffer[scratchSize];

// Kind of a hack for parsing. We only need a type of the correct number of bytes.
typedef struct { u8  v1; } Bytes1;
typedef struct { u16 v1; } Bytes2;
typedef struct { u32 v1; } Bytes4;
typedef struct { u32 v1, v2; } Bytes8;
typedef struct { u32 v1, v2, v3; } Bytes12;
typedef struct { u32 v1, v2, v3, v4; } Bytes16;
typedef struct { u32 v1, v2, v3, v4, v5; } Bytes20;
typedef struct { u32 v1, v2, v3, v4, v5, v6; } Bytes24;
typedef struct { u32 v1, v2, v3, v4, v5, v6, v7; } Bytes28;
typedef struct { u32 v1, v2, v3, v4, v5, v6, v7, v8; } Bytes32;

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> Result AllocateNewObject(DebugStreamClient::Object &newObject, const s32 additionalBytesRequired, void ** dataSegment, s32 &dataLength, MemoryStack scratch)
    {
      char *innerObjectName = reinterpret_cast<char*>( scratch.Allocate(SerializedBuffer::DESCRIPTION_STRING_LENGTH + 1) );

      Type objectOfType;

      {
        void * dataSegment_tmp = *dataSegment;
        s32 dataLength_tmp = dataLength;
        objectOfType.Deserialize(innerObjectName, &dataSegment_tmp, dataLength_tmp, scratch);
      }

      newObject.bufferLength = additionalBytesRequired + objectOfType.get_serializationSize();
      newObject.buffer = malloc(newObject.bufferLength);

      if(!newObject.buffer)
        return RESULT_FAIL;

      newObject.startOfPayload = newObject.buffer;

      MemoryStack localMemory(reinterpret_cast<u8*>(newObject.buffer) + sizeof(objectOfType), newObject.bufferLength - sizeof(objectOfType));

      objectOfType.Deserialize(innerObjectName, dataSegment, dataLength, localMemory);

      memcpy(newObject.startOfPayload, &objectOfType, sizeof(objectOfType));

      return RESULT_OK;
    }

    DebugStreamClient::Object::Object()
      : bufferLength(0), buffer(NULL), startOfPayload(NULL)
    {
    }

    bool DebugStreamClient::Object::IsValid() const
    {
      if(bufferLength <= 0)
        return false;

      return true;
    }

    Result DebugStreamClient::Close()
    {
      this->isRunning = false;

      // Wait for the threads to complete
      while(this->isConnectionThreadActive && this->isParseBufferThreadActive)
      {}

      return RESULT_OK;
    }

    DebugStreamClient::DebugStreamClient(const char * ipAddress, const s32 port)
      : isRunning(true), ipAddress(ipAddress), port(port), isConnectionThreadActive(false), isParseBufferThreadActive(false)
    {
      //printf("Starting DebugStreamClient\n");

      rawMessageQueue = ThreadSafeQueue<RawBuffer>();

      //printf("Connection opened\n");

#ifdef _MSC_VER
      DWORD connectionThreadId = -1;
      CreateThread(
        NULL,        // default security attributes
        0,           // use default stack size
        DebugStreamClient::ConnectionThread, // thread function name
        this,    // argument to thread function
        0,           // use default creation flags
        &connectionThreadId);  // returns the thread identifier
#else
      pthread_t connectionThread;
      pthread_attr_t connectionAttr;
      pthread_attr_init(&connectionAttr);

      pthread_create(&connectionThread, &connectionAttr, DebugStreamClient::ConnectionThread, (void *)this);
#endif
      //printf("Connection thread created\n");

#ifdef _MSC_VER
      DWORD parsingThreadId = -1;
      CreateThread(
        NULL,        // default security attributes
        0,           // use default stack size
        DebugStreamClient::ParseBufferThread, // thread function name
        this,    // argument to thread function
        0,           // use default creation flags
        &parsingThreadId);  // returns the thread identifier
#else // #ifdef _MSC_VER
      pthread_t parsingThread;
      pthread_attr_t parsingAttr;
      pthread_attr_init(&parsingAttr);

      pthread_create(&parsingThread, &parsingAttr, DebugStreamClient::ParseBufferThread, (void *)this);
#endif // #ifdef _MSC_VER ... else
      //printf("Parsing thread created\n");
    } // DebugStreamClient::DebugStreamClient

    DebugStreamClient::Object DebugStreamClient::GetNextObject()
    {
      DebugStreamClient::Object newObject;

      while(parsedObjectQueue.IsEmpty()) {
#ifdef _MSC_VER
        Sleep(10);
#else
        usleep(10000);
#endif
      }

      newObject = parsedObjectQueue.Pop();

      return newObject;
    }

    bool DebugStreamClient::get_isRunning() const
    {
      return this->isRunning;
    }

    DebugStreamClient::RawBuffer DebugStreamClient::AllocateNewRawBuffer(const s32 bufferRawSize)
    {
      RawBuffer rawBuffer;

      rawBuffer.rawDataPointer = reinterpret_cast<u8*>(malloc(bufferRawSize));
      rawBuffer.data = reinterpret_cast<u8*>( RoundUp(reinterpret_cast<size_t>(rawBuffer.rawDataPointer), MEMORY_ALIGNMENT) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH );
      rawBuffer.maxDataLength = bufferRawSize - (reinterpret_cast<size_t>(rawBuffer.data) - reinterpret_cast<size_t>(rawBuffer.rawDataPointer));
      rawBuffer.curDataLength = 0;
      rawBuffer.timeReceived = 0;

      if(rawBuffer.rawDataPointer == NULL) {
        //printf("Could not allocate memory");
        rawBuffer.data = NULL;
        rawBuffer.maxDataLength = 0;
      }

      return rawBuffer;
    }

    void DebugStreamClient::ProcessRawBuffer(RawBuffer &buffer, ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue, const bool requireMatchingSegmentLengths)
    {
      MemoryStack scratch(scratchBuffer, scratchSize, Flags::Buffer(false, true, false));

      char *innerObjectName = reinterpret_cast<char*>( scratch.Allocate(SerializedBuffer::DESCRIPTION_STRING_LENGTH + 1) );

      SerializedBuffer serializedBuffer(buffer.data, buffer.curDataLength, Anki::Embedded::Flags::Buffer(false, true, true));

      SerializedBufferReconstructingIterator iterator(serializedBuffer);

      bool aMessageAlreadyPrinted = false;

      while(iterator.HasNext()) {
        s32 dataLength;
        const char * typeName = NULL;
        const char * objectName = NULL;
        bool isReportedSegmentLengthCorrect;
        void * dataSegment = reinterpret_cast<u8*>(iterator.GetNext(&typeName, &objectName, dataLength, isReportedSegmentLengthCorrect));

        if(!dataSegment) {
          break;
        }

        if(requireMatchingSegmentLengths && !isReportedSegmentLengthCorrect) {
          continue;
        }

        Object newObject;

        newObject.timeReceived = buffer.timeReceived;

        // Copy the type and object names. Probably the user will use these to figure out how to cast the payload
        memcpy(newObject.typeName, typeName, SerializedBuffer::DESCRIPTION_STRING_LENGTH);
        memcpy(newObject.objectName, objectName, SerializedBuffer::DESCRIPTION_STRING_LENGTH);

        newObject.typeName[SerializedBuffer::DESCRIPTION_STRING_LENGTH - 1] = '\0';
        newObject.objectName[SerializedBuffer::DESCRIPTION_STRING_LENGTH - 1] = '\0';

        //printf("Next segment is (%d,%d): ", dataLength, type);
        if(strcmp(typeName, "Basic Type Buffer") == 0) {
          u16 sizeOfType;
          bool isBasicType;
          bool isInteger;
          bool isSigned;
          bool isFloat;
          s32 numElements;
          void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          s32 tmpDataLength = dataLength - 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          SerializedBuffer::EncodedBasicTypeBuffer::Deserialize(false, sizeOfType, isBasicType, isInteger, isSigned, isFloat, numElements, &tmpDataSegment, tmpDataLength);

          //printf("Basic type buffer segment \"%s\" (%d, %d, %d, %d, %d)\n", objectName, sizeOfType, isInteger, isSigned, isFloat, numElements);

          newObject.bufferLength = 512 + static_cast<s32>(sizeOfType) * numElements;
          newObject.buffer = malloc(newObject.bufferLength);

          if(!newObject.buffer)
            continue;

          // Copy the header (probably the user won't need this, but just in case)
          reinterpret_cast<s32*>(newObject.buffer)[0] = sizeOfType;
          reinterpret_cast<s32*>(newObject.buffer)[1] = isBasicType;
          reinterpret_cast<s32*>(newObject.buffer)[2] = isInteger;
          reinterpret_cast<s32*>(newObject.buffer)[3] = isSigned;
          reinterpret_cast<s32*>(newObject.buffer)[4] = isFloat;
          reinterpret_cast<s32*>(newObject.buffer)[5] = numElements;

          MemoryStack localMemory(reinterpret_cast<void*>(&reinterpret_cast<s32*>(newObject.buffer)[6]), newObject.bufferLength - 6*sizeof(s32));

          // the type may not be a char, but we only care about the start of the buffer (this only works for basic types c-style arrays, not for Array objects)
          newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<char>(innerObjectName, &dataSegment, dataLength, localMemory);
        } else if(strcmp(typeName, "Array") == 0) {
          s32 height;
          s32 width;
          s32 stride;
          Flags::Buffer flags;
          u16 basicType_sizeOfType;
          bool basicType_isBasicType;
          bool basicType_isInteger;
          bool basicType_isSigned;
          bool basicType_isFloat;
          s32 basicType_numElements;
          void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          s32 tmpDataLength = dataLength - 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          SerializedBuffer::EncodedArray::Deserialize(false, height, width, stride, flags, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_numElements, &tmpDataSegment, tmpDataLength);

          newObject.bufferLength = 512 + stride * height;
          newObject.buffer = malloc(newObject.bufferLength);

          if(!newObject.buffer)
            continue;

          // Copy the header (probably the user won't need this, but just in case)
          reinterpret_cast<s32*>(newObject.buffer)[0] = height;
          reinterpret_cast<s32*>(newObject.buffer)[1] = width;
          reinterpret_cast<s32*>(newObject.buffer)[2] = stride;
          reinterpret_cast<u32*>(newObject.buffer)[3] = flags.get_rawFlags();
          reinterpret_cast<s32*>(newObject.buffer)[4] = basicType_sizeOfType;
          reinterpret_cast<s32*>(newObject.buffer)[5] = basicType_isBasicType;
          reinterpret_cast<s32*>(newObject.buffer)[6] = basicType_isInteger;
          reinterpret_cast<s32*>(newObject.buffer)[7] = basicType_isSigned;
          reinterpret_cast<s32*>(newObject.buffer)[8] = basicType_isFloat;
          reinterpret_cast<s32*>(newObject.buffer)[9] = basicType_numElements;

          newObject.startOfPayload = reinterpret_cast<void*>(&reinterpret_cast<s32*>(newObject.buffer)[10]);

          MemoryStack localMemory(reinterpret_cast<void*>(reinterpret_cast<u8*>(newObject.buffer)+256), newObject.bufferLength - 256);

          // This next part is a little wierd.
          // We only need to get the sizeOfType correct. Nothing else about the type is stored explicitly in the array.
          // If we have the type size correct, then everything will be initialized and copied correctly, so when the Array is cast to the correct Type, everything will be fine.

          if(basicType_sizeOfType == 1) {
            Array<Bytes1> arr = SerializedBuffer::DeserializeRawArray<Bytes1>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 2) {
            Array<Bytes2> arr = SerializedBuffer::DeserializeRawArray<Bytes2>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 4) {
            Array<Bytes4> arr = SerializedBuffer::DeserializeRawArray<Bytes4>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 8) {
            Array<Bytes8> arr = SerializedBuffer::DeserializeRawArray<Bytes8>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 12) {
            Array<Bytes12> arr = SerializedBuffer::DeserializeRawArray<Bytes12>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 16) {
            Array<Bytes16> arr = SerializedBuffer::DeserializeRawArray<Bytes16>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 20) {
            Array<Bytes20> arr = SerializedBuffer::DeserializeRawArray<Bytes20>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 24) {
            Array<Bytes24> arr = SerializedBuffer::DeserializeRawArray<Bytes24>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 28) {
            Array<Bytes28> arr = SerializedBuffer::DeserializeRawArray<Bytes28>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 32) {
            Array<Bytes32> arr = SerializedBuffer::DeserializeRawArray<Bytes32>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            //printf("Unusual size %d. Add a case to parse objects of this Type.\n", basicType_sizeOfType);
            continue;
          }
        } else if(strcmp(typeName, "ArraySlice") == 0) {
          s32 height;
          s32 width;
          s32 stride;
          Flags::Buffer flags;
          s32 ySlice_start;
          s32 ySlice_increment;
          s32 ySlice_end;
          s32 xSlice_start;
          s32 xSlice_increment;
          s32 xSlice_end;
          u16 basicType_sizeOfType;
          bool basicType_isBasicType;
          bool basicType_isInteger;
          bool basicType_isSigned;
          bool basicType_isFloat;
          s32 basicType_numElements;
          void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          SerializedBuffer::EncodedArraySlice::Deserialize(false, height, width, stride, flags, ySlice_start, ySlice_increment, ySlice_end, xSlice_start, xSlice_increment, xSlice_end, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_numElements, &tmpDataSegment, dataLength);

          newObject.bufferLength = 512 + stride * height;
          newObject.buffer = malloc(newObject.bufferLength);

          if(!newObject.buffer)
            continue;

          // Copy the header (probably the user won't need this, but just in case)
          reinterpret_cast<s32*>(newObject.buffer)[0] = height;
          reinterpret_cast<s32*>(newObject.buffer)[1] = width;
          reinterpret_cast<s32*>(newObject.buffer)[2] = stride;
          reinterpret_cast<u32*>(newObject.buffer)[3] = flags.get_rawFlags();
          reinterpret_cast<s32*>(newObject.buffer)[4] = basicType_sizeOfType;
          reinterpret_cast<s32*>(newObject.buffer)[5] = basicType_isBasicType;
          reinterpret_cast<s32*>(newObject.buffer)[6] = basicType_isInteger;
          reinterpret_cast<s32*>(newObject.buffer)[7] = basicType_isSigned;
          reinterpret_cast<s32*>(newObject.buffer)[8] = basicType_isFloat;
          reinterpret_cast<s32*>(newObject.buffer)[9] = basicType_numElements;

          newObject.startOfPayload = reinterpret_cast<void*>(&reinterpret_cast<s32*>(newObject.buffer)[10]);

          MemoryStack localMemory(reinterpret_cast<void*>(reinterpret_cast<u8*>(newObject.buffer)+256), newObject.bufferLength - 256);

          // As with Array<>, this next part is a little wierd.
          // We only need to get the sizeOfType correct. Nothing else about the type is stored explicitly in the array.
          // If we have the type size correct, then everything will be initialized and copied correctly, so when the Array is cast to the correct Type, everything will be fine.

          if(basicType_sizeOfType == 1) {
            ArraySlice<Bytes1> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes1>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 2) {
            ArraySlice<Bytes2> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes2>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 4) {
            ArraySlice<Bytes4> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes4>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 8) {
            ArraySlice<Bytes8> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes8>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 12) {
            ArraySlice<Bytes12> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes12>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 16) {
            ArraySlice<Bytes16> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes16>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 20) {
            ArraySlice<Bytes20> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes20>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 24) {
            ArraySlice<Bytes24> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes24>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 28) {
            ArraySlice<Bytes28> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes28>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else if(basicType_sizeOfType == 32) {
            ArraySlice<Bytes32> arr = SerializedBuffer::DeserializeRawArraySlice<Bytes32>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            //printf("Unusual size %d. Add a case to parse objects of this Type.\n", basicType_sizeOfType);
            continue;
          }
        } else if(strcmp(typeName, "String") == 0) {
          const s32 stringLength = strlen(reinterpret_cast<char*>(dataSegment));

          newObject.bufferLength = 32 + stringLength;
          newObject.buffer = malloc(newObject.bufferLength);

          if(!newObject.buffer)
            continue;

          reinterpret_cast<s32*>(newObject.buffer)[0] = stringLength;

          newObject.startOfPayload = reinterpret_cast<void*>(&reinterpret_cast<s32*>(newObject.buffer)[1]);

          memcpy(newObject.startOfPayload, dataSegment, stringLength);
          reinterpret_cast<char*>(newObject.startOfPayload)[stringLength] = '\0';
        } else if(strcmp(typeName, "VisionMarker") == 0) {
          if(AllocateNewObject<VisionMarker>(newObject, 32, &dataSegment, dataLength, scratch) != RESULT_OK)
            continue;
        } else if(strcmp(typeName, "PlanarTransformation_f32") == 0) {
          if(AllocateNewObject<Transformations::PlanarTransformation_f32>(newObject, 4196, &dataSegment, dataLength, scratch) != RESULT_OK)
            continue;
        } else if(strcmp(typeName, "BinaryTracker") == 0) {
          if(AllocateNewObject<TemplateTracker::BinaryTracker>(newObject, 4196, &dataSegment, dataLength, scratch) != RESULT_OK)
            continue;
        } else if(strcmp(typeName, "EdgeLists") == 0) {
          if(AllocateNewObject<EdgeLists>(newObject, 4196, &dataSegment, dataLength, scratch) != RESULT_OK)
            continue;
        } else {
          newObject.bufferLength = 0;
          newObject.buffer = NULL;
        }

        if(newObject.IsValid())
          parsedObjectQueue.Push(newObject);

        //printf("Received %s %s\n", newObject.typeName, newObject.objectName);
      } // while(iterator.HasNext())

      free(buffer.rawDataPointer);
      buffer.rawDataPointer = NULL;
      buffer.data = NULL;

      return;
    } // void ProcessRawBuffer()

    ThreadResult DebugStreamClient::ConnectionThread(void *threadParameter)
    {
#ifdef _MSC_VER
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); // THREAD_PRIORITY_ABOVE_NORMAL
#else
#endif

      u8 *usbBuffer = reinterpret_cast<u8*>(malloc(USB_BUFFER_SIZE));
      RawBuffer nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);

      if(!usbBuffer || !nextRawBuffer.data) {
        //AnkiError("DebugStreamClient::ConnectionThread", "Could not allocate usbBuffer and nextRawBuffer.data");
#ifdef _MSC_VER
        return -1;
#else
        return NULL;
#endif
      }

      Object object;
      object.bufferLength = -1;

      DebugStreamClient *callingObject = (DebugStreamClient *) threadParameter;

      callingObject->isConnectionThreadActive = true;

      Socket socket;

      while(socket.Open(callingObject->ipAddress, callingObject->port) != RESULT_OK) {
        //printf("Trying again to open socket.\n");
#ifdef _MSC_VER
        Sleep(1000);
#else
        usleep(1000000);
#endif
      }

      bool atLeastOneStartFound = false;
      s32 start_state = 0;

      while(callingObject->get_isRunning()) {
        s32 bytesRead = 0;

        while(socket.Read(usbBuffer, USB_BUFFER_SIZE-2, bytesRead) != RESULT_OK)
        {
          //printf("socket read failure. Retrying...\n");
#ifdef _MSC_VER
          Sleep(1);
#else
          usleep(1000);
#endif
        }

        if(bytesRead == 0) {
#ifdef _MSC_VER
          Sleep(1);
#else
          usleep(1000);
#endif
          continue;
        }

        // Find the next SERIALIZED_BUFFER_HEADER
        s32 start_searchIndex = 0;
        s32 start_foundIndex = -1;

        // This method can only find one message per usbBuffer
        // TODO: support more
        while(start_searchIndex < static_cast<s32>(bytesRead)) {
          if(start_state == SERIALIZED_BUFFER_HEADER_LENGTH) {
            start_foundIndex = start_searchIndex;
            start_state = 0;
            break;
          }

          if(usbBuffer[start_searchIndex] == SERIALIZED_BUFFER_HEADER[start_state]) {
            start_state++;
          } else if(usbBuffer[start_searchIndex] == SERIALIZED_BUFFER_HEADER[0]) {
            start_state = 1;
          } else {
            start_state = 0;
          }

          start_searchIndex++;
        } // while(start_searchIndex < static_cast<s32>(bytesRead))

        // If we found a start header, handle it
        if(start_foundIndex != -1) {
          if(atLeastOneStartFound) {
            const s32 numBytesToCopy = start_foundIndex;

            if((nextRawBuffer.curDataLength + numBytesToCopy + 16) > nextRawBuffer.maxDataLength) {
              nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
              //printf("Buffer trashed\n");
              continue;
            }

            memcpy(
              nextRawBuffer.data + nextRawBuffer.curDataLength,
              usbBuffer,
              numBytesToCopy);

            nextRawBuffer.curDataLength += numBytesToCopy;
            nextRawBuffer.timeReceived = GetTimeF64();

            callingObject->rawMessageQueue.Push(nextRawBuffer);

            nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
          } else {
            atLeastOneStartFound = true;
          }

          const s32 numBytesToCopy = static_cast<s32>(bytesRead) - start_foundIndex;

          // If we've filled up the buffer, just trash it
          if((nextRawBuffer.curDataLength + numBytesToCopy + 16) > nextRawBuffer.maxDataLength) {
            nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
            //printf("Buffer trashed\n");
            continue;
          }

          if(numBytesToCopy <= 0) {
            //printf("negative numBytesToCopy");
            continue;
          }

          memcpy(
            nextRawBuffer.data + nextRawBuffer.curDataLength,
            usbBuffer + start_foundIndex,
            numBytesToCopy);

          nextRawBuffer.curDataLength += numBytesToCopy;
        } else {// if(start_foundIndex != -1)
          if(atLeastOneStartFound) {
            const s32 numBytesToCopy = static_cast<s32>(bytesRead);

            // If we've filled up the buffer, just trash it
            if((nextRawBuffer.curDataLength + numBytesToCopy + 16) > nextRawBuffer.maxDataLength) {
              nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
              //printf("Buffer trashed\n");
              continue;
            }

            memcpy(
              nextRawBuffer.data + nextRawBuffer.curDataLength,
              usbBuffer,
              numBytesToCopy);

            nextRawBuffer.curDataLength += numBytesToCopy;
          }
        }
      } // while(this->isConnected)

      socket.Close();

      callingObject->isConnectionThreadActive = false;

      return 0;
    }

    ThreadResult DebugStreamClient::ParseBufferThread(void *threadParameter)
    {
#ifdef _MSC_VER
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
      // TODO: set thread priority
#endif

      DebugStreamClient *callingObject = (DebugStreamClient *) threadParameter;

      callingObject->isParseBufferThreadActive = true;

      ThreadSafeQueue<RawBuffer> &rawMessageQueue = callingObject->rawMessageQueue;
      ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue = callingObject->parsedObjectQueue;

      while(true) {
        while(rawMessageQueue.IsEmpty() && callingObject->get_isRunning()) {
#ifdef _MSC_VER
          Sleep(1);
#else
          usleep(1000);
#endif
        }

        if(!callingObject->get_isRunning())
          break;

        RawBuffer nextRawBuffer = rawMessageQueue.Pop();

        ProcessRawBuffer(nextRawBuffer, parsedObjectQueue, false);
      } // while(true)

      callingObject->isParseBufferThreadActive = false;

      return 0;
    }
  } // namespace Embedded
} // namespace Anki
