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

#include "coretech/vision/robot/fiducialMarkers.h"
#include "coretech/vision/robot/binaryTracker.h"

#include <ctime>
#include <vector>

#undef printf
#define printf (Do not use printf on this thread)

using namespace std;

static const s32 scratchSize = 1000000;
static u8 scratchBuffer[scratchSize];

// Kind of a hack for parsing. We only need a type of the correct number of bytes.
template<s32 num> struct Bytes { u8 v[num]; };

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
        Result result = objectOfType.Deserialize(innerObjectName, &dataSegment_tmp, dataLength_tmp, scratch);

        if(result != RESULT_OK)
          return result;
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

    template <s32 num> Result CopyPayload_Array(void * startOfPayload, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      Array<Bytes<num> > arr = SerializedBuffer::DeserializeRawArray<Bytes<num> >(NULL, buffer, bufferLength, memory);

      if(!arr.IsValid())
        return RESULT_FAIL;

      memcpy(startOfPayload, &arr, sizeof(arr));

      return RESULT_OK;
    }

    template <s32 num> Result CopyPayload_ArraySlice(void * startOfPayload, void ** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      ArraySlice<Bytes<num> > arr = SerializedBuffer::DeserializeRawArraySlice<Bytes<num> >(NULL, buffer, bufferLength, memory);

      if(!arr.IsValid())
        return RESULT_FAIL;

      memcpy(startOfPayload, &arr, sizeof(arr));

      return RESULT_OK;
    }

    DebugStreamClient::Object::Object()
      : buffer(NULL), bufferLength(0), startOfPayload(NULL)
    {
    }

    bool DebugStreamClient::Object::IsValid() const
    {
      if(bufferLength <= 0)
        return false;

      return true;
    }

    DebugStreamClient::ObjectToSave::ObjectToSave()
      : DebugStreamClient::Object()
    {
    }

    DebugStreamClient::ObjectToSave::ObjectToSave(DebugStreamClient::Object &object, const char * filename)
    {
      this->buffer = object.buffer;
      this->bufferLength = object.bufferLength;
      this->startOfPayload = object.startOfPayload;
      this->timeReceived = object.timeReceived;

      snprintf(this->typeName, Anki::Embedded::SerializedBuffer::DESCRIPTION_STRING_LENGTH, "%s", object.typeName);
      snprintf(this->objectName, Anki::Embedded::SerializedBuffer::DESCRIPTION_STRING_LENGTH, "%s", object.objectName);
      snprintf(this->filename, Anki::Embedded::DebugStreamClient::ObjectToSave::SAVE_FILENAME_PATTERN_LENGTH, "%s", filename);
    }

    Result DebugStreamClient::Close()
    {
      this->isRunning = false;

      if(this->isValid) {
        // Wait for the threads to complete
#ifdef _MSC_VER
        WaitForSingleObject(connectionThread, INFINITE);
        WaitForSingleObject(parseBufferThread, INFINITE);
        WaitForSingleObject(saveObjectThread, INFINITE);
#else
        pthread_join(connectionThread, NULL);
        pthread_join(parseBufferThread, NULL);
        pthread_join(saveObjectThread, NULL);
#endif
      }

      this->isValid = false;

      // Clean up allocated memory
      rawMessageQueue.Lock();
      while(rawMessageQueue.Size_unsafe() > 0) {
        DebugStreamClient::RawBuffer object = rawMessageQueue.Front_unsafe();
        rawMessageQueue.Pop_unsafe();
        free(object.rawDataPointer);
      }
      rawMessageQueue.Unlock();

      parsedObjectQueue.Lock();
      while(parsedObjectQueue.Size_unsafe() > 0) {
        DebugStreamClient::Object object = parsedObjectQueue.Front_unsafe();
        parsedObjectQueue.Pop_unsafe();
        free(object.buffer);
      }
      parsedObjectQueue.Unlock();

      saveObjectQueue.Lock();
      while(saveObjectQueue.Size_unsafe() > 0) {
        DebugStreamClient::ObjectToSave object = saveObjectQueue.Front_unsafe();
        saveObjectQueue.Pop_unsafe();
        free(object.buffer);
      }
      saveObjectQueue.Unlock();

      return RESULT_OK;
    }

    DebugStreamClient::DebugStreamClient(const char * ipAddress, const s32 port)
      : isValid(false), isSocket(true), socket_ipAddress(ipAddress), socket_port(port)
    {
      Initialize();
    } // DebugStreamClient::DebugStreamClient

    DebugStreamClient::DebugStreamClient(const s32 comPort, const s32 baudRate, const char parity, const s32 dataBits, const s32 stopBits)
      : isValid(false), isSocket(false), serial_comPort(comPort), serial_baudRate(baudRate), serial_parity(parity), serial_dataBits(dataBits), serial_stopBits(stopBits)
    {
      Initialize();
    }

    DebugStreamClient::~DebugStreamClient()
    {
      this->Close();
    }

    Result DebugStreamClient::Initialize()
    {
      this->isRunning = true;

      //CoreTechPrint("Starting DebugStreamClient\n");

      this->rawMessageQueue = ThreadSafeQueue<DebugStreamClient::RawBuffer>();
      this->parsedObjectQueue = ThreadSafeQueue<DebugStreamClient::Object>();
      this->saveObjectQueue = ThreadSafeQueue<DebugStreamClient::ObjectToSave>();

#ifdef _MSC_VER
      DWORD connectionThreadId = -1;
      connectionThread = CreateThread(
        NULL,        // default security attributes
        0,           // use default stack size
        DebugStreamClient::ConnectionThread, // thread function name
        this,    // argument to thread function
        0,           // use default creation flags
        &connectionThreadId);  // returns the thread identifier
#else
      pthread_attr_t connectionAttr;
      pthread_attr_init(&connectionAttr);

      pthread_create(&connectionThread, &connectionAttr, DebugStreamClient::ConnectionThread, (void *)this);
#endif
      //CoreTechPrint("Connection thread created\n");

#ifdef _MSC_VER
      DWORD parseBufferThreadId = -1;
      parseBufferThread = CreateThread(
        NULL,        // default security attributes
        0,           // use default stack size
        DebugStreamClient::ParseBufferThread, // thread function name
        this,    // argument to thread function
        0,           // use default creation flags
        &parseBufferThreadId);  // returns the thread identifier
#else // #ifdef _MSC_VER
      pthread_attr_t parsingAttr;
      pthread_attr_init(&parsingAttr);

      pthread_create(&parseBufferThread, &parsingAttr, DebugStreamClient::ParseBufferThread, (void *)this);
#endif // #ifdef _MSC_VER ... else
      //CoreTechPrint("Parsing thread created\n");

#ifdef _MSC_VER
      DWORD saveObjectThreadId = -1;
      saveObjectThread = CreateThread(
        NULL,        // default security attributes
        0,           // use default stack size
        DebugStreamClient::SaveObjectThread, // thread function name
        this,    // argument to thread function
        0,           // use default creation flags
        &saveObjectThreadId);  // returns the thread identifier
#else // #ifdef _MSC_VER
      pthread_attr_t savingAttr;
      pthread_attr_init(&savingAttr);

      pthread_create(&saveObjectThread, &savingAttr, DebugStreamClient::SaveObjectThread, (void *)this);
#endif // #ifdef _MSC_VER ... else
      //CoreTechPrint("Saving thread created\n");

      this->isValid = true;

      return RESULT_OK;
    }

    DebugStreamClient::Object DebugStreamClient::GetNextObject(s32 maxAttempts)
    {
      DebugStreamClient::Object newObject;

      bool foundObject = true;
      s32 attempts = 0;
      while(true) {
        parsedObjectQueue.Lock();

        if(parsedObjectQueue.Size_unsafe() > 0) {
          parsedObjectQueue.Unlock();
          break;
        }

        parsedObjectQueue.Unlock();

        usleep(10000);

        ++attempts;
        if(attempts >= maxAttempts) {
          foundObject = false;
          break;
        }
      }

      if(foundObject) {
        parsedObjectQueue.Lock();
        newObject = parsedObjectQueue.Front_unsafe();
        parsedObjectQueue.Pop_unsafe();
        parsedObjectQueue.Unlock();
      } else {
        newObject = DebugStreamClient::Object();
      }

      return newObject;
    }

    Result DebugStreamClient::SaveObject(Object &object, const char * filename)
    {
      DebugStreamClient::ObjectToSave toSave(object, filename);

      saveObjectQueue.Lock();
      saveObjectQueue.Push_unsafe(toSave);
      saveObjectQueue.Unlock();

      return RESULT_OK;
    }

    bool DebugStreamClient::get_isRunning() const
    {
      return this->isRunning;
    }

    DebugStreamClient::RawBuffer DebugStreamClient::AllocateNewRawBuffer(const s32 bufferRawSize)
    {
      DebugStreamClient::RawBuffer rawBuffer;

      rawBuffer.rawDataPointer = reinterpret_cast<u8*>(malloc(bufferRawSize));
      rawBuffer.data = reinterpret_cast<u8*>( RoundUp(reinterpret_cast<size_t>(rawBuffer.rawDataPointer), MEMORY_ALIGNMENT) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH );
      rawBuffer.maxDataLength = bufferRawSize - static_cast<s32>(reinterpret_cast<size_t>(rawBuffer.data) - reinterpret_cast<size_t>(rawBuffer.rawDataPointer));
      rawBuffer.curDataLength = 0;
      rawBuffer.timeReceived = 0;

      if(rawBuffer.rawDataPointer == NULL) {
        //CoreTechPrint("Could not allocate memory");
        rawBuffer.data = NULL;
        rawBuffer.maxDataLength = 0;
      }

      return rawBuffer;
    }

    void DebugStreamClient::ProcessRawBuffer(DebugStreamClient::RawBuffer &buffer, ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue, const bool requireMatchingSegmentLengths)
    {
      MemoryStack scratch(scratchBuffer, scratchSize, Flags::Buffer(false, true, false));

      char *innerObjectName = reinterpret_cast<char*>( scratch.Allocate(SerializedBuffer::DESCRIPTION_STRING_LENGTH + 1) );

      SerializedBuffer serializedBuffer(buffer.data, buffer.curDataLength, Anki::Embedded::Flags::Buffer(false, true, true));

      SerializedBufferReconstructingIterator iterator(serializedBuffer);

      //bool aMessageAlreadyPrinted = false;

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

        //CoreTechPrint("Next segment is (%d,%d): ", dataLength, type);
        if(strcmp(typeName, "Basic Type Buffer") == 0) {
          u16 sizeOfType;
          bool isBasicType;
          bool isInteger;
          bool isSigned;
          bool isFloat;
          bool isString;
          s32 numElements;
          void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          s32 tmpDataLength = dataLength - 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          SerializedBuffer::EncodedBasicTypeBuffer::Deserialize(false, sizeOfType, isBasicType, isInteger, isSigned, isFloat, isString, numElements, &tmpDataSegment, tmpDataLength);

          //CoreTechPrint("Basic type buffer segment \"%s\" (%d, %d, %d, %d, %d)\n", objectName, sizeOfType, isInteger, isSigned, isFloat, numElements);

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
          bool basicType_isString;
          s32 basicType_numElements;
          void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          s32 tmpDataLength = dataLength - 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          SerializedBuffer::EncodedArray::Deserialize(false, height, width, stride, flags, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString, basicType_numElements, &tmpDataSegment, tmpDataLength);

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

          Result result;

          if(basicType_sizeOfType == 1) { result = CopyPayload_Array<1>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 2) { result = CopyPayload_Array<2>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 4) { result = CopyPayload_Array<4>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 8) { result = CopyPayload_Array<8>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 12) { result = CopyPayload_Array<12>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 16) { result = CopyPayload_Array<16>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 20) { result = CopyPayload_Array<20>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 24) { result = CopyPayload_Array<24>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 28) { result = CopyPayload_Array<28>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 32) { result = CopyPayload_Array<32>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 36) { result = CopyPayload_Array<36>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 40) { result = CopyPayload_Array<40>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 44) { result = CopyPayload_Array<44>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 48) { result = CopyPayload_Array<48>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 52) { result = CopyPayload_Array<52>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 56) { result = CopyPayload_Array<56>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 60) { result = CopyPayload_Array<60>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 64) { result = CopyPayload_Array<64>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 68) { result = CopyPayload_Array<68>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 72) { result = CopyPayload_Array<72>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 76) { result = CopyPayload_Array<76>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 80) { result = CopyPayload_Array<80>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 84) { result = CopyPayload_Array<84>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 88) { result = CopyPayload_Array<88>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 92) { result = CopyPayload_Array<92>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 96) { result = CopyPayload_Array<96>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 100) { result = CopyPayload_Array<100>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 104) { result = CopyPayload_Array<104>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 108) { result = CopyPayload_Array<108>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 112) { result = CopyPayload_Array<112>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 116) { result = CopyPayload_Array<116>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 120) { result = CopyPayload_Array<120>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 124) { result = CopyPayload_Array<124>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 128) { result = CopyPayload_Array<128>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 132) { result = CopyPayload_Array<132>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 136) { result = CopyPayload_Array<136>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 140) { result = CopyPayload_Array<140>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 144) { result = CopyPayload_Array<144>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 148) { result = CopyPayload_Array<148>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 152) { result = CopyPayload_Array<152>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 156) { result = CopyPayload_Array<156>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 160) { result = CopyPayload_Array<160>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 164) { result = CopyPayload_Array<164>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 168) { result = CopyPayload_Array<168>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 172) { result = CopyPayload_Array<172>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 176) { result = CopyPayload_Array<176>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 180) { result = CopyPayload_Array<180>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 184) { result = CopyPayload_Array<184>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 188) { result = CopyPayload_Array<188>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 192) { result = CopyPayload_Array<192>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 196) { result = CopyPayload_Array<196>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 200) { result = CopyPayload_Array<200>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 204) { result = CopyPayload_Array<204>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 208) { result = CopyPayload_Array<208>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 212) { result = CopyPayload_Array<212>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 216) { result = CopyPayload_Array<216>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 220) { result = CopyPayload_Array<220>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 224) { result = CopyPayload_Array<224>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 228) { result = CopyPayload_Array<228>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 232) { result = CopyPayload_Array<232>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 236) { result = CopyPayload_Array<236>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 240) { result = CopyPayload_Array<240>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 244) { result = CopyPayload_Array<244>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 248) { result = CopyPayload_Array<248>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 252) { result = CopyPayload_Array<252>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 256) { result = CopyPayload_Array<256>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else {
            //CoreTechPrint("Unusual size %d. Add a case to parse objects of this Type.\n", basicType_sizeOfType);
            continue;
          }

          if(result != RESULT_OK)
            continue;
        } else if(strcmp(typeName, "ArraySlice") == 0) {
          s32 height;
          s32 width;
          s32 stride;
          Flags::Buffer flags;
          s32 ySlice_start;
          s32 ySlice_increment;
          s32 ySlice_size;
          s32 xSlice_start;
          s32 xSlice_increment;
          s32 xSlice_size;
          u16 basicType_sizeOfType;
          bool basicType_isBasicType;
          bool basicType_isInteger;
          bool basicType_isSigned;
          bool basicType_isFloat;
          bool basicType_isString;
          s32 basicType_numElements;
          void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
          SerializedBuffer::EncodedArraySlice::Deserialize(false, height, width, stride, flags, ySlice_start, ySlice_increment, ySlice_size, xSlice_start, xSlice_increment, xSlice_size, basicType_sizeOfType, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_isString, basicType_numElements, &tmpDataSegment, dataLength);

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

          Result result;

          if(basicType_sizeOfType == 1) { result = CopyPayload_ArraySlice<1>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 2) { result = CopyPayload_ArraySlice<2>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 4) { result = CopyPayload_ArraySlice<4>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 8) { result = CopyPayload_ArraySlice<8>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 12) { result = CopyPayload_ArraySlice<12>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 16) { result = CopyPayload_ArraySlice<16>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 20) { result = CopyPayload_ArraySlice<20>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 24) { result = CopyPayload_ArraySlice<24>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 28) { result = CopyPayload_ArraySlice<28>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 32) { result = CopyPayload_ArraySlice<32>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 36) { result = CopyPayload_ArraySlice<36>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 40) { result = CopyPayload_ArraySlice<40>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 44) { result = CopyPayload_ArraySlice<44>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 48) { result = CopyPayload_ArraySlice<48>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 52) { result = CopyPayload_ArraySlice<52>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 56) { result = CopyPayload_ArraySlice<56>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 60) { result = CopyPayload_ArraySlice<60>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 64) { result = CopyPayload_ArraySlice<64>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 68) { result = CopyPayload_ArraySlice<68>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 72) { result = CopyPayload_ArraySlice<72>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 76) { result = CopyPayload_ArraySlice<76>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 80) { result = CopyPayload_ArraySlice<80>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 84) { result = CopyPayload_ArraySlice<84>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 88) { result = CopyPayload_ArraySlice<88>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 92) { result = CopyPayload_ArraySlice<92>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 96) { result = CopyPayload_ArraySlice<96>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 100) { result = CopyPayload_ArraySlice<100>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 104) { result = CopyPayload_ArraySlice<104>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 108) { result = CopyPayload_ArraySlice<108>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 112) { result = CopyPayload_ArraySlice<112>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 116) { result = CopyPayload_ArraySlice<116>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 120) { result = CopyPayload_ArraySlice<120>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 124) { result = CopyPayload_ArraySlice<124>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 128) { result = CopyPayload_ArraySlice<128>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 132) { result = CopyPayload_ArraySlice<132>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 136) { result = CopyPayload_ArraySlice<136>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 140) { result = CopyPayload_ArraySlice<140>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 144) { result = CopyPayload_ArraySlice<144>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 148) { result = CopyPayload_ArraySlice<148>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 152) { result = CopyPayload_ArraySlice<152>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 156) { result = CopyPayload_ArraySlice<156>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 160) { result = CopyPayload_ArraySlice<160>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 164) { result = CopyPayload_ArraySlice<164>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 168) { result = CopyPayload_ArraySlice<168>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 172) { result = CopyPayload_ArraySlice<172>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 176) { result = CopyPayload_ArraySlice<176>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 180) { result = CopyPayload_ArraySlice<180>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 184) { result = CopyPayload_ArraySlice<184>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 188) { result = CopyPayload_ArraySlice<188>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 192) { result = CopyPayload_ArraySlice<192>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 196) { result = CopyPayload_ArraySlice<196>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 200) { result = CopyPayload_ArraySlice<200>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 204) { result = CopyPayload_ArraySlice<204>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 208) { result = CopyPayload_ArraySlice<208>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 212) { result = CopyPayload_ArraySlice<212>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 216) { result = CopyPayload_ArraySlice<216>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 220) { result = CopyPayload_ArraySlice<220>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 224) { result = CopyPayload_ArraySlice<224>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 228) { result = CopyPayload_ArraySlice<228>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 232) { result = CopyPayload_ArraySlice<232>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 236) { result = CopyPayload_ArraySlice<236>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 240) { result = CopyPayload_ArraySlice<240>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 244) { result = CopyPayload_ArraySlice<244>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 248) { result = CopyPayload_ArraySlice<248>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 252) { result = CopyPayload_ArraySlice<252>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else if(basicType_sizeOfType == 256) { result = CopyPayload_ArraySlice<256>(newObject.startOfPayload, &dataSegment, dataLength, localMemory); }
          else {
            //CoreTechPrint("Unusual size %d. Add a case to parse objects of this Type.\n", basicType_sizeOfType);
            continue;
          }

          if(result != RESULT_OK)
            continue;
        } else if(strcmp(typeName, "String") == 0) {
          const size_t stringLength = strlen(reinterpret_cast<char*>(dataSegment));
          AnkiAssert(stringLength <= s32_MAX);

          newObject.bufferLength = 32 + static_cast<s32>(stringLength);
          newObject.buffer = malloc(newObject.bufferLength);

          if(!newObject.buffer)
            continue;

          reinterpret_cast<s32*>(newObject.buffer)[0] = static_cast<s32>(stringLength);

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

        if(newObject.IsValid()) {
          parsedObjectQueue.Lock();
          parsedObjectQueue.Push_unsafe(newObject);
          parsedObjectQueue.Unlock();
        }

        //CoreTechPrint("Received %s %s\n", newObject.typeName, newObject.objectName);
      } // while(iterator.HasNext())

      free(buffer.rawDataPointer);
      buffer.rawDataPointer = NULL;
      buffer.data = NULL;

      return;
    } // void ProcessRawBuffer()

    static void WaitForOpenSocket(const char * ipAddress, const s32 port, Socket &socket)
    {
      while(socket.Open(ipAddress, port) != RESULT_OK) {
        //CoreTechPrint("Trying again to open socket.\n");
        usleep(1000000);
      }
    }

    ThreadResult DebugStreamClient::ConnectionThread(void *threadParameter)
    {
#ifdef _MSC_VER
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#else
#endif

      u8 *usbBuffer = reinterpret_cast<u8*>(malloc(CONNECTION_BUFFER_SIZE));
      DebugStreamClient::RawBuffer nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);

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

      Socket socket;
      Serial serial;

      if(callingObject->isSocket) {
        WaitForOpenSocket(callingObject->socket_ipAddress, callingObject->socket_port, socket);
      } else {
        while(serial.Open(callingObject->serial_comPort, callingObject->serial_baudRate, callingObject->serial_parity, callingObject->serial_dataBits, callingObject->serial_stopBits) != RESULT_OK) {
          //CoreTechPrint("Trying again to open serial.\n");
          usleep(1000000);
        }
      }

      bool atLeastOneStartFound = false;
      s32 start_state = 0;

      while(callingObject->get_isRunning()) {
        s32 bytesRead = 0;

        if(callingObject->isSocket) {
          Result lastError;
          while((lastError = socket.Read(usbBuffer, CONNECTION_BUFFER_SIZE-2, bytesRead)) != RESULT_OK)
          {
            //CoreTechPrint("Socket read error. Attempting to reconnect...\n");
            if(lastError == RESULT_FAIL_IO_TIMEOUT) {
              usleep(1000);
            } else {
              WaitForOpenSocket(callingObject->socket_ipAddress, callingObject->socket_port, socket);
            }
          }
        } else {
          while(serial.Read(usbBuffer, CONNECTION_BUFFER_SIZE-2, bytesRead) != RESULT_OK)
          {
            //CoreTechPrint("serial read failure. Retrying...\n");
            usleep(1000);
          }
        }

        if(bytesRead == 0) {
          usleep(1000);
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
              //CoreTechPrint("Buffer trashed\n");
              continue;
            }

            memcpy(
              nextRawBuffer.data + nextRawBuffer.curDataLength,
              usbBuffer,
              numBytesToCopy);

            nextRawBuffer.curDataLength += numBytesToCopy;
            nextRawBuffer.timeReceived = GetTimeF64();

            callingObject->rawMessageQueue.Lock();
            callingObject->rawMessageQueue.Push_unsafe(nextRawBuffer);
            callingObject->rawMessageQueue.Unlock();

            nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
          } else {
            atLeastOneStartFound = true;
          }

          const s32 numBytesToCopy = static_cast<s32>(bytesRead) - start_foundIndex;

          // If we've filled up the buffer, just trash it
          if((nextRawBuffer.curDataLength + numBytesToCopy + 16) > nextRawBuffer.maxDataLength) {
            nextRawBuffer = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
            //CoreTechPrint("Buffer trashed\n");
            continue;
          }

          if(numBytesToCopy <= 0) {
            //CoreTechPrint("negative numBytesToCopy");
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
              //CoreTechPrint("Buffer trashed\n");
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

      if(callingObject->isSocket) {
        socket.Close();
      } else {
        serial.Close();
      }

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

      ThreadSafeQueue<DebugStreamClient::RawBuffer> &rawMessageQueue = callingObject->rawMessageQueue;
      ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue = callingObject->parsedObjectQueue;

      while(true) {
        while(true) {
          if(!callingObject->get_isRunning())
            break;

          rawMessageQueue.Lock();

          if(rawMessageQueue.Size_unsafe() > 0) {
            rawMessageQueue.Unlock();
            break;
          }

          rawMessageQueue.Unlock();

          usleep(1000);
        }

        if(!callingObject->get_isRunning())
          break;

        rawMessageQueue.Lock();
        DebugStreamClient::RawBuffer nextRawBuffer = rawMessageQueue.Front_unsafe();
        rawMessageQueue.Pop_unsafe();
        rawMessageQueue.Unlock();

        ProcessRawBuffer(nextRawBuffer, parsedObjectQueue, false);
      } // while(true)

      return 0;
    }

    ThreadResult DebugStreamClient::SaveObjectThread(void *threadParameter)
    {
#ifdef _MSC_VER
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
#else
      // TODO: set thread priority
#endif

      DebugStreamClient *callingObject = (DebugStreamClient *) threadParameter;

      ThreadSafeQueue<DebugStreamClient::ObjectToSave> &saveObjectQueue = callingObject->saveObjectQueue;

      while(true) {
        while(true) {
          if(!callingObject->get_isRunning())
            break;

          saveObjectQueue.Lock();

          if(saveObjectQueue.Size_unsafe() > 0) {
            saveObjectQueue.Unlock();
            break;
          }

          saveObjectQueue.Unlock();

          usleep(10000);
        }

        if(!callingObject->get_isRunning())
          break;

        saveObjectQueue.Lock();
        const DebugStreamClient::ObjectToSave nextObject = saveObjectQueue.Front_unsafe();
        saveObjectQueue.Pop_unsafe();
        saveObjectQueue.Unlock();

        // TODO: save things other than images
        Array<u8> image = *(reinterpret_cast<Array<u8>*>(nextObject.startOfPayload));

        vector<int> compression_params;
        compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
        compression_params.push_back(9);

        cv::Mat_<u8> image_cvMat;
        ArrayToCvMat(image, &image_cvMat);

        cv::imwrite(nextObject.filename, image_cvMat, compression_params);

        free(nextObject.buffer);
      } // while(true)

      return 0;
    }
  } // namespace Embedded
} // namespace Anki
