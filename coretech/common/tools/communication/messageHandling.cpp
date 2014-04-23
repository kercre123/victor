/**
File: messageHandling.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "messageHandling.h"

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

using namespace Anki::Embedded;
using namespace std;

static const s32 scratchSize = 1000000;
static u8 scratchBuffer[scratchSize];

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

  MemoryStack localMemory(newObject.buffer, newObject.bufferLength);

  objectOfType.Deserialize(innerObjectName, dataSegment, dataLength, localMemory);

  memcpy(newObject.startOfPayload, &objectOfType, sizeof(objectOfType));

  return RESULT_OK;
}

DebugStreamClient::Object::Object()
  : bufferLength(0), buffer(NULL), startOfPayload(NULL)
{
}

//DebugStreamClient::Object::Object(void * buffer, const s32 bufferLength)
//  : bufferLength(bufferLength), buffer(buffer), startOfPayload(NULL)
//{
//}

bool DebugStreamClient::Object::IsValid() const
{
  if(bufferLength <= 0)
    return false;

  return true;
}

Result DebugStreamClient::Close()
{
  if(isConnected) {
    if(socket.Close() != RESULT_OK) {
      return RESULT_FAIL_IO;
    }
  }

  this->isConnected = false;

  return RESULT_OK;
}

DebugStreamClient::DebugStreamClient(const char * ipAddress, const s32 port)
  : isConnected(false)
{
  //printf("Starting DebugStreamClient\n");

  rawMessageQueue = ThreadSafeQueue<DisplayRawBuffer>();

  while(socket.Open(ipAddress, port) != RESULT_OK) {
    //printf("Trying again to open socket.\n");
#ifdef _MSC_VER
    Sleep(100);
#else
    usleep(100000);
#endif
  }

  //printf("Connection opened\n");

#ifdef _MSC_VER
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST); // THREAD_PRIORITY_ABOVE_NORMAL
#else
#endif

#ifdef _MSC_VER
  DWORD parsingThreadId = -1;
  CreateThread(
    NULL,        // default security attributes
    0,           // use default stack size
    DebugStreamClient::ParseBufferThread, // thread function name
    this,    // argument to thread function
    0,           // use default creation flags
    &parsingThreadId);  // returns the thread identifier
#else
  // TODO: set thread priority

  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  pthread_create(&thread, &attr, DebugStreamClient::ParseBufferThread, (void *)this);
#endif

  //printf("Parsing thread created\n");

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
  // TODO: set thread priority

  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  pthread_create(&thread, &attr, DebugStreamClient::ConnectionThread, (void *)this);
#endif

  //printf("Connection thread created\n");

  bool atLeastOneStartFound = false;
  s32 start_state = 0;

  this->isConnected = true;
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

bool DebugStreamClient::get_isConnected() const
{
  return this->isConnected;
}

DebugStreamClient::DisplayRawBuffer DebugStreamClient::AllocateNewRawBuffer(const s32 bufferRawSize)
{
  DisplayRawBuffer rawBuffer;

  rawBuffer.rawDataPointer = reinterpret_cast<u8*>(malloc(bufferRawSize));
  rawBuffer.data = reinterpret_cast<u8*>( RoundUp(reinterpret_cast<size_t>(rawBuffer.rawDataPointer), MEMORY_ALIGNMENT) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH );
  rawBuffer.maxDataLength = bufferRawSize - (reinterpret_cast<size_t>(rawBuffer.data) - reinterpret_cast<size_t>(rawBuffer.rawDataPointer));
  rawBuffer.curDataLength = 0;

  if(rawBuffer.rawDataPointer == NULL) {
    //printf("Could not allocate memory");
    rawBuffer.data = NULL;
    rawBuffer.maxDataLength = 0;
  }

  return rawBuffer;
}

void DebugStreamClient::ProcessRawBuffer(DisplayRawBuffer &buffer, ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue, const bool requireMatchingSegmentLengths)
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

    // Copy the type and object names. Probably the user will use these to figure out how to cast the payload
    memcpy(newObject.typeName, typeName, SerializedBuffer::DESCRIPTION_STRING_LENGTH);
    memcpy(newObject.objectName, objectName, SerializedBuffer::DESCRIPTION_STRING_LENGTH);

    newObject.typeName[SerializedBuffer::DESCRIPTION_STRING_LENGTH - 1] = '\0';
    newObject.objectName[SerializedBuffer::DESCRIPTION_STRING_LENGTH - 1] = '\0';

    //printf("Next segment is (%d,%d): ", dataLength, type);
    if(strcmp(typeName, "Basic Type Buffer") == 0) {
      u16 size;
      bool isBasicType;
      bool isInteger;
      bool isSigned;
      bool isFloat;
      s32 numElements;

      SerializedBuffer::EncodedBasicTypeBuffer::Deserialize(false, size, isBasicType, isInteger, isSigned, isFloat, numElements, &dataSegment, dataLength);

      //printf("Basic type buffer segment \"%s\" (%d, %d, %d, %d, %d)\n", objectName, size, isInteger, isSigned, isFloat, numElements);

      newObject.bufferLength = 512 + static_cast<s32>(size) * numElements;
      newObject.buffer = malloc(newObject.bufferLength);

      if(!newObject.buffer)
        continue;

      // Copy the header (probably the user won't need this, but just in case)
      reinterpret_cast<s32*>(newObject.buffer)[0] = size;
      reinterpret_cast<s32*>(newObject.buffer)[1] = isBasicType;
      reinterpret_cast<s32*>(newObject.buffer)[2] = isInteger;
      reinterpret_cast<s32*>(newObject.buffer)[3] = isSigned;
      reinterpret_cast<s32*>(newObject.buffer)[4] = isFloat;
      reinterpret_cast<s32*>(newObject.buffer)[5] = numElements;

      MemoryStack localMemory(reinterpret_cast<void*>(&reinterpret_cast<s32*>(newObject.buffer)[6]), newObject.bufferLength - 6*sizeof(s32));

      // TODO: make a DeserializeRawBasicType function that is not templated, so we don't need to repeat these, and so an unknown type can be handled
      if(isFloat) {
        if(size == 4) {
          newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<f32>(innerObjectName, &dataSegment, dataLength, localMemory);
        } else if(size == 8) {
          newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<f64>(innerObjectName, &dataSegment, dataLength, localMemory);
        } else {
          //printf("Could not parse basic type buffer %s\n", objectName);
        }
      } else { // if(isFloat)
        if(size == 1) {
          if(isSigned) {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<s8>(innerObjectName, &dataSegment, dataLength, localMemory);
          } else {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<u8>(innerObjectName, &dataSegment, dataLength, localMemory);
          }
        } else if(size == 2) {
          if(isSigned) {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<s16>(innerObjectName, &dataSegment, dataLength, localMemory);
          } else {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<u16>(innerObjectName, &dataSegment, dataLength, localMemory);
          }
        } else if(size == 4) {
          if(isSigned) {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<s32>(innerObjectName, &dataSegment, dataLength, localMemory);
          } else {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<u32>(innerObjectName, &dataSegment, dataLength, localMemory);
          }
        } else if(size == 8) {
          if(isSigned) {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<s64>(innerObjectName, &dataSegment, dataLength, localMemory);
          } else {
            newObject.startOfPayload = SerializedBuffer::DeserializeRawBasicType<u64>(innerObjectName, &dataSegment, dataLength, localMemory);
          }
        } else {
          //printf("Could not parse basic type buffer %s\n", objectName);
        }
      } // if(isFloat) ... else
    } else if(strcmp(typeName, "Array") == 0) {
      s32 height;
      s32 width;
      s32 stride;
      Flags::Buffer flags;
      u16 basicType_size;
      bool basicType_isBasicType;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      s32 basicType_numElements;
      void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
      SerializedBuffer::EncodedArray::Deserialize(false, height, width, stride, flags, basicType_size, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_numElements, &tmpDataSegment, dataLength);

      newObject.bufferLength = 512 + stride * height;
      newObject.buffer = malloc(newObject.bufferLength);

      if(!newObject.buffer)
        continue;

      // Copy the header (probably the user won't need this, but just in case)
      reinterpret_cast<s32*>(newObject.buffer)[0] = height;
      reinterpret_cast<s32*>(newObject.buffer)[1] = width;
      reinterpret_cast<s32*>(newObject.buffer)[2] = stride;
      reinterpret_cast<u32*>(newObject.buffer)[3] = flags.get_rawFlags();
      reinterpret_cast<s32*>(newObject.buffer)[4] = basicType_size;
      reinterpret_cast<s32*>(newObject.buffer)[5] = basicType_isBasicType;
      reinterpret_cast<s32*>(newObject.buffer)[6] = basicType_isInteger;
      reinterpret_cast<s32*>(newObject.buffer)[7] = basicType_isSigned;
      reinterpret_cast<s32*>(newObject.buffer)[8] = basicType_isFloat;
      reinterpret_cast<s32*>(newObject.buffer)[9] = basicType_numElements;

      newObject.startOfPayload = reinterpret_cast<void*>(&reinterpret_cast<s32*>(newObject.buffer)[10]);

      MemoryStack localMemory(reinterpret_cast<void*>(reinterpret_cast<u8*>(newObject.buffer)+256), newObject.bufferLength - 256);

      // TODO: make a DeserializeRawBasicType function that is not templated, so we don't need to repeat these, and so an unknown type can be handled
      if(basicType_isFloat) {
        if(basicType_size == 4) {
          Array<f32> arr = SerializedBuffer::DeserializeRawArray<f32>(NULL, &dataSegment, dataLength, localMemory);

          if(!arr.IsValid())
            continue;

          memcpy(newObject.startOfPayload, &arr, sizeof(arr));
        } else if(basicType_size == 8) {
          Array<f64> arr = SerializedBuffer::DeserializeRawArray<f64>(NULL, &dataSegment, dataLength, localMemory);

          if(!arr.IsValid())
            continue;

          memcpy(newObject.startOfPayload, &arr, sizeof(arr));
        } else {
          //printf("Could not parse Array %s\n", objectName);
        }
      } else { // if(basicType_isFloat)
        if(basicType_size == 1) {
          if(basicType_isSigned) {
            Array<s8> arr = SerializedBuffer::DeserializeRawArray<s8>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            Array<u8> arr = SerializedBuffer::DeserializeRawArray<u8>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else if(basicType_size == 2) {
          if(basicType_isSigned) {
            Array<s16> arr = SerializedBuffer::DeserializeRawArray<s16>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            Array<u16> arr = SerializedBuffer::DeserializeRawArray<u16>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else if(basicType_size == 4) {
          if(basicType_isSigned) {
            Array<s32> arr = SerializedBuffer::DeserializeRawArray<s32>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            Array<u32> arr = SerializedBuffer::DeserializeRawArray<u32>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else if(basicType_size == 8) {
          if(basicType_isSigned) {
            Array<s64> arr = SerializedBuffer::DeserializeRawArray<s64>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            Array<u64> arr = SerializedBuffer::DeserializeRawArray<u64>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else {
          //printf("Could not parse Array %s\n", objectName);
        }
      } // if(basicType_isFloat) ... else
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
      u16 basicType_size;
      bool basicType_isBasicType;
      bool basicType_isInteger;
      bool basicType_isSigned;
      bool basicType_isFloat;
      s32 basicType_numElements;
      void * tmpDataSegment = reinterpret_cast<u8*>(dataSegment) + 2*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
      SerializedBuffer::EncodedArraySlice::Deserialize(false, height, width, stride, flags, ySlice_start, ySlice_increment, ySlice_end, xSlice_start, xSlice_increment, xSlice_end, basicType_size, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat, basicType_numElements, &tmpDataSegment, dataLength);

      newObject.bufferLength = 512 + stride * height;
      newObject.buffer = malloc(newObject.bufferLength);

      if(!newObject.buffer)
        continue;

      // Copy the header (probably the user won't need this, but just in case)
      reinterpret_cast<s32*>(newObject.buffer)[0] = height;
      reinterpret_cast<s32*>(newObject.buffer)[1] = width;
      reinterpret_cast<s32*>(newObject.buffer)[2] = stride;
      reinterpret_cast<u32*>(newObject.buffer)[3] = flags.get_rawFlags();
      reinterpret_cast<s32*>(newObject.buffer)[4] = basicType_size;
      reinterpret_cast<s32*>(newObject.buffer)[5] = basicType_isBasicType;
      reinterpret_cast<s32*>(newObject.buffer)[6] = basicType_isInteger;
      reinterpret_cast<s32*>(newObject.buffer)[7] = basicType_isSigned;
      reinterpret_cast<s32*>(newObject.buffer)[8] = basicType_isFloat;
      reinterpret_cast<s32*>(newObject.buffer)[9] = basicType_numElements;

      newObject.startOfPayload = reinterpret_cast<void*>(&reinterpret_cast<s32*>(newObject.buffer)[10]);

      MemoryStack localMemory(reinterpret_cast<void*>(reinterpret_cast<u8*>(newObject.buffer)+256), newObject.bufferLength - 256);

      // TODO: make a DeserializeRawBasicType function that is not templated, so we don't need to repeat these, and so an unknown type can be handled
      if(basicType_isFloat) {
        if(basicType_size == 4) {
          ArraySlice<f32> arr = SerializedBuffer::DeserializeRawArraySlice<f32>(NULL, &dataSegment, dataLength, localMemory);

          if(!arr.get_array().IsValid())
            continue;

          memcpy(newObject.startOfPayload, &arr, sizeof(arr));
        } else if(basicType_size == 8) {
          ArraySlice<f64> arr = SerializedBuffer::DeserializeRawArraySlice<f64>(NULL, &dataSegment, dataLength, localMemory);

          if(!arr.get_array().IsValid())
            continue;

          memcpy(newObject.startOfPayload, &arr, sizeof(arr));
        } else {
          //printf("Could not parse Array %s\n", objectName);
        }
      } else { // if(basicType_isFloat)
        if(basicType_size == 1) {
          if(basicType_isSigned) {
            ArraySlice<s8> arr = SerializedBuffer::DeserializeRawArraySlice<s8>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            ArraySlice<u8> arr = SerializedBuffer::DeserializeRawArraySlice<u8>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else if(basicType_size == 2) {
          if(basicType_isSigned) {
            ArraySlice<s16> arr = SerializedBuffer::DeserializeRawArraySlice<s16>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            ArraySlice<u16> arr = SerializedBuffer::DeserializeRawArraySlice<u16>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else if(basicType_size == 4) {
          if(basicType_isSigned) {
            ArraySlice<s32> arr = SerializedBuffer::DeserializeRawArraySlice<s32>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            ArraySlice<u32> arr = SerializedBuffer::DeserializeRawArraySlice<u32>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else if(basicType_size == 8) {
          if(basicType_isSigned) {
            ArraySlice<s64> arr = SerializedBuffer::DeserializeRawArraySlice<s64>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          } else {
            ArraySlice<u64> arr = SerializedBuffer::DeserializeRawArraySlice<u64>(NULL, &dataSegment, dataLength, localMemory);

            if(!arr.get_array().IsValid())
              continue;

            memcpy(newObject.startOfPayload, &arr, sizeof(arr));
          }
        } else {
          //printf("Could not parse Array %s\n", objectName);
        }
      } // if(basicType_isFloat) ... else
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

  //if(aMessageAlreadyPrinted) {
  //  printf("\n");
  //}

  //if(lastImage.rows > 0) {
  //  // std::queue<VisionMarker> visionMarkerList;
  //  //Quadrilateral<s16> corners; // SQ 15.0 (Though may be changed later)
  //  //Vision::MarkerType markerType;
  //  //bool isValid;

  //  // Print FPS
  //  if(benchmarkTimes[0] > 0.0f) {
  //    char benchmarkBuffer[1024];

  //    static f32 lastTime;
  //    const f32 curTime = GetTime();
  //    const f32 receivedDelta = curTime - lastTime;
  //    lastTime = GetTime();

  //    //snprintf(benchmarkBuffer, 1024, "Total:%dfps Algorithms:%dfps Received:%dfps", Round<s32>(1.0f/benchmarkTimes[1]), Round<s32>(1.0f/benchmarkTimes[0]), Round<s32>(1.0f/receivedDelta));
  //    snprintf(benchmarkBuffer, 1024, "Total:%0.1ffps Algorithms:%0.1ffps   GrayvalueError:%d %f", 1.0f/benchmarkTimes[1], 1.0f/benchmarkTimes[0], lastMeanError, lastPercentMatchingGrayvalues);

  //    cv::putText(toShowImage, benchmarkBuffer, cv::Point(5,15), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0,255,0));
  //  }

  //  if(isTracking) {
  //    cv::Mat trackingBoxImage(bigHeight, bigWidth, CV_8UC3);

  //    trackingBoxImage.setTo(0);

  //    const cv::Scalar textColor    = cv::Scalar(0,255,0);
  //    //const cv::Scalar boxColor = cv::Scalar(0,128,0);
  //    const cv::Scalar boxColor     = cv::Scalar(0,16,0);
  //    const cv::Scalar topLineColor = cv::Scalar(16,0,0);

  //    //const Quadrilateral<f32> transformedCorners = lastPlanarTransformation.get_transformedCorners(scratch);
  //    //const Quadrilateral<f32> sortedCorners = transformedCorners.ComputeClockwiseCorners<f32>();

  //    const Quadrilateral<f32> orientedCorners = lastPlanarTransformation.get_transformedCorners(scratch).ComputeRotatedCorners<f32>(0.0f);

  //    const s32 cornerOrder[5] = {
  //      Quadrilateral<f32>::TopLeft,
  //      Quadrilateral<f32>::TopRight,
  //      Quadrilateral<f32>::BottomRight,
  //      Quadrilateral<f32>::BottomLeft,
  //      Quadrilateral<f32>::TopLeft};

  //    for(s32 iCorner=0; iCorner<4; iCorner++) {
  //      const s32 point1Index = cornerOrder[iCorner];
  //      const s32 point2Index = cornerOrder[iCorner+1];
  //      const cv::Point pt1(Round<s32>(orientedCorners[point1Index].x*scale), Round<s32>(orientedCorners[point1Index].y*scale));
  //      const cv::Point pt2(Round<s32>(orientedCorners[point2Index].x*scale), Round<s32>(orientedCorners[point2Index].y*scale));

  //      cv::Scalar thisLineColor = (iCorner==0) ? topLineColor : boxColor;
  //      cv::line(trackingBoxImage, pt1, pt2, thisLineColor, 15);
  //    }

  //    const Point<f32> center = orientedCorners.ComputeCenter<f32>();
  //    const s32 textX = Round<s32>(MIN(MIN(MIN(orientedCorners.corners[0].x*scale, orientedCorners.corners[1].x*scale), orientedCorners.corners[2].x*scale), orientedCorners.corners[3].x*scale));
  //    const cv::Point textStartPoint(textX, Round<s32>(center.y*scale));

  //    cv::putText(trackingBoxImage, "Tracking", textStartPoint, cv::FONT_HERSHEY_PLAIN, 1.0, textColor);

  //    const s32 numPixels = bigHeight * bigWidth * 3;

  //    for(s32 iPixel=0; iPixel<numPixels; iPixel++) {
  //      toShowImage.data[iPixel] += trackingBoxImage.data[iPixel];
  //    }
  //  } else { // if(isTracking)
  //    // Draw markers
  //    for(s32 iMarker=0; iMarker<static_cast<s32>(visionMarkerList.size()); iMarker++) {
  //      cv::Scalar boxColor, topLineColor, textColor;
  //      if(visionMarkerList[iMarker].isValid) {
  //        textColor = cv::Scalar(0,255,0);
  //        boxColor = cv::Scalar(0,128,0);
  //      } else {
  //        textColor = cv::Scalar(0,0,255);
  //        boxColor = cv::Scalar(0,0,128);
  //      }
  //      topLineColor = cv::Scalar(128,0,0);

  //      const f32 observedOrientation = visionMarkerList[iMarker].observedOrientation;
  //      //const Quadrilateral<f32> orientedCorners = visionMarkerList[iMarker].corners.ComputeRotatedCorners<f32>(observedOrientation*PI/180);
  //      const Quadrilateral<f32> orientedCorners = visionMarkerList[iMarker].corners.ComputeRotatedCorners<f32>(0.0f);

  //      const s32 cornerOrder[5] = {
  //        Quadrilateral<f32>::TopLeft,
  //        Quadrilateral<f32>::TopRight,
  //        Quadrilateral<f32>::BottomRight,
  //        Quadrilateral<f32>::BottomLeft,
  //        Quadrilateral<f32>::TopLeft};

  //      for(s32 iCorner=0; iCorner<4; iCorner++) {
  //        const s32 point1Index = cornerOrder[iCorner];
  //        const s32 point2Index = cornerOrder[iCorner+1];
  //        const cv::Point pt1(Round<s32>(orientedCorners[point1Index].x*scale), Round<s32>(orientedCorners[point1Index].y*scale));
  //        const cv::Point pt2(Round<s32>(orientedCorners[point2Index].x*scale), Round<s32>(orientedCorners[point2Index].y*scale));

  //        cv::Scalar thisLineColor = (iCorner==0) ? topLineColor : boxColor;
  //        cv::line(toShowImage, pt1, pt2, thisLineColor, 2);
  //      }

  //      const Anki::Vision::MarkerType markerType = visionMarkerList[iMarker].markerType;

  //      const char * typeString = "??";
  //      if(static_cast<s32>(markerType) >=0 && static_cast<s32>(markerType) <= Anki::Vision::NUM_MARKER_TYPES) {
  //        typeString = Anki::Vision::MarkerTypeStrings[markerType];
  //      }

  //      const Point<s16> center = visionMarkerList[iMarker].corners.ComputeCenter<s16>();
  //      const s32 textX = Round<s32>(MIN(MIN(MIN(visionMarkerList[iMarker].corners[0].x*scale, visionMarkerList[iMarker].corners[1].x*scale), visionMarkerList[iMarker].corners[2].x*scale), visionMarkerList[iMarker].corners[3].x*scale));
  //      const cv::Point textStartPoint(textX, Round<s32>(center.y*scale));

  //      cv::putText(toShowImage, typeString, textStartPoint, cv::FONT_HERSHEY_PLAIN, 1.0, textColor);
  //    }
  //  } // if(isTracking) ... else

  //  if(detectedFaces.size() != 0) {
  //    const f32 faceScale = static_cast<f32>(bigWidth) / static_cast<f32>(detectedFacesImageWidth);
  //    for( s32 i = 0; i < static_cast<s32>(detectedFaces.size()); i++ )
  //    {
  //      cv::Point center( Round<s32>(faceScale*(detectedFaces[i].left + detectedFaces[i].right)*0.5), Round<s32>(faceScale*(detectedFaces[i].top + detectedFaces[i].bottom)*0.5) );
  //      cv::ellipse( toShowImage, center, cv::Size( Round<s32>(faceScale*(detectedFaces[i].right-detectedFaces[i].left)*0.5), Round<s32>(faceScale*(detectedFaces[i].bottom-detectedFaces[i].top)*0.5)), 0, 0, 360, cv::Scalar( 255, 0, 0 ), 5, 8, 0 );
  //    }
  //  }

  //  cv::imshow("Robot Image", toShowImage);
  //  const s32 pressedKey = cv::waitKey(10);
  //  //printf("%d\n", pressedKey);
  //  if(pressedKey == 'c') {
  //    const time_t t = time(0);   // get time now
  //    const struct tm * currentTime = localtime(&t);
  //    char outputFilename[1024];
  //    snprintf(&outputFilename[0], 1024, outputFilenamePattern,
  //      currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
  //      currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
  //      0,
  //      "png");

  //    printf("Saving to %s\n", outputFilename);
  //    cv::imwrite(outputFilename, lastImage);
  //  }
  //} else { // if(lastImage.rows > 0)
  //  cv::waitKey(1);
  //}

  free(buffer.rawDataPointer);
  buffer.rawDataPointer = NULL;
  buffer.data = NULL;

  return;
} // void ProcessRawBuffer()

ThreadResult DebugStreamClient::ConnectionThread(void *threadParameter)
{
  Object object;
  object.bufferLength = -1;

  DebugStreamClient *callingObject = (DebugStreamClient *) threadParameter;

  if(!callingObject->isConnected) {
    //AnkiError("DebugStreamClient::ConnectionThread", "Not connected");
    return -1;
  }

  u8 *usbBuffer = reinterpret_cast<u8*>(malloc(USB_BUFFER_SIZE));
  DisplayRawBuffer nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);

  if(!usbBuffer || !nextMessage.data) {
    //AnkiError("DebugStreamClient::ConnectionThread", "Could not allocate usbBuffer and nextMessage.data");
    return -2;
  }

  bool atLeastOneStartFound = false;
  s32 start_state = 0;

  while(callingObject->get_isConnected()) {
    if(!usbBuffer) {
      //AnkiError("DebugStreamClient::ConnectionThread", "Could not allocate usbBuffer");
      return -3;
    }

    s32 bytesRead = 0;

    while(callingObject->socket.Read(usbBuffer, USB_BUFFER_SIZE-2, bytesRead) != RESULT_OK)
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

        if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
          nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
          //printf("Buffer trashed\n");
          continue;
        }

        memcpy(
          nextMessage.data + nextMessage.curDataLength,
          usbBuffer,
          numBytesToCopy);

        nextMessage.curDataLength += numBytesToCopy;

        //ProcessRawBuffer(nextMessage, true, false);
        callingObject->rawMessageQueue.Push(nextMessage);

        nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
      } else {
        atLeastOneStartFound = true;
      }

      const s32 numBytesToCopy = static_cast<s32>(bytesRead) - start_foundIndex;

      // If we've filled up the buffer, just trash it
      if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
        nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
        //printf("Buffer trashed\n");
        continue;
      }

      if(numBytesToCopy <= 0) {
        //printf("negative numBytesToCopy");
        continue;
      }

      memcpy(
        nextMessage.data + nextMessage.curDataLength,
        usbBuffer + start_foundIndex,
        numBytesToCopy);

      nextMessage.curDataLength += numBytesToCopy;
    } else {// if(start_foundIndex != -1)
      if(atLeastOneStartFound) {
        const s32 numBytesToCopy = static_cast<s32>(bytesRead);

        // If we've filled up the buffer, just trash it
        if((nextMessage.curDataLength + numBytesToCopy + 16) > nextMessage.maxDataLength) {
          nextMessage = AllocateNewRawBuffer(MESSAGE_BUFFER_SIZE);
          //printf("Buffer trashed\n");
          continue;
        }

        memcpy(
          nextMessage.data + nextMessage.curDataLength,
          usbBuffer,
          numBytesToCopy);

        nextMessage.curDataLength += numBytesToCopy;
      }
    }
  } // while(this->isConnected)

  if(callingObject->socket.Close() != RESULT_OK)
    return -5;

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

  ThreadSafeQueue<DisplayRawBuffer> &rawMessageQueue = callingObject->rawMessageQueue;
  ThreadSafeQueue<DebugStreamClient::Object> &parsedObjectQueue = callingObject->parsedObjectQueue;

  while(true) {
    while(rawMessageQueue.IsEmpty()) {
#ifdef _MSC_VER
      Sleep(1);
#else
      usleep(1000);
#endif

      cv::waitKey(1);
    }

    DisplayRawBuffer nextMessage = rawMessageQueue.Pop();

    ProcessRawBuffer(nextMessage, parsedObjectQueue, false);
  }

  return 0;
}
