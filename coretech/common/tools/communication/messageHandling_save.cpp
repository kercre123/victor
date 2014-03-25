/**
File: messageHandling_save.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "messageHandling.h"

#include "anki/common/robot/config.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/serialize.h"

#include "anki/vision/robot/fiducialMarkers.h"

#include <ctime>
#include <vector>

#include "opencv/cv.h"
#include "opencv2/imgproc/imgproc.hpp"

using namespace Anki::Embedded;
using namespace std;

#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

#ifndef ANKICORETECH_EMBEDDED_USE_OPENCV
#error OpenCV is required
#endif

void ProcessRawBuffer_Save(RawBuffer &buffer, const string outputFilenamePattern, const bool freeBuffer, const bool requireCRCmatch)
{
  const s32 outputFilenameLength = 1024;
  char outputFilename[outputFilenameLength];

  u8 * const bufferDataOriginal = buffer.data;

  static u8 bigBufferRaw2[BIG_BUFFER_SIZE];
  u8 * const shiftedBufferOriginal = reinterpret_cast<u8*>(malloc(BIG_BUFFER_SIZE));
  u8 * shiftedBuffer = shiftedBufferOriginal;

  const time_t t = time(0);   // get time now
  const struct tm * currentTime = localtime(&t);

  s32 frameNumber = 0;

  MemoryStack memory(bigBufferRaw2, BIG_BUFFER_SIZE, Flags::Buffer(false, true, false));

  // Used for displaying detected fiducials
  cv::Mat lastImage(240,320,CV_8U);
  lastImage.setTo(0);

  bool isTracking = false;
  Transformations::PlanarTransformation_f32 lastPlanarTransformation(Transformations::TRANSFORM_PROJECTIVE, memory);

  std::vector<VisionMarker> visionMarkerList;

#ifdef PRINTF_ALL_RECEIVED
  printf("\n");
  //for(s32 i=0; i<buffer.dataLength; i++) {
  for(s32 i=0; i<500; i++) {
    printf("%x ", buffer.data[i]);
  }
  printf("\n");
#endif

  s32 bufferDataOffset = 0;
  //bool atLeastOneHeaderFound = false;
  while(bufferDataOffset < (buffer.dataLength - SERIALIZED_BUFFER_HEADER_LENGTH - SERIALIZED_BUFFER_FOOTER_LENGTH))
  {
    s32 usbMessageStartIndex;
    s32 usbMessageEndIndex;
    SerializedBuffer::FindSerializedBuffer(buffer.data+bufferDataOffset, buffer.dataLength-bufferDataOffset, usbMessageStartIndex, usbMessageEndIndex);

    if(usbMessageStartIndex < 0) {
      printf("Error: USB header is missing (%d,%d) with %d total bytes and %d bytes remaining, returning...\n", usbMessageStartIndex, usbMessageEndIndex, buffer.dataLength, buffer.dataLength-bufferDataOffset);

      for(s32 i=0; i<15; i++) {
        printf("%x ", *(buffer.data+bufferDataOffset+i));
      }
      printf("\n");

      if(freeBuffer) {
        buffer.data = NULL;
        free(bufferDataOriginal);
        free(shiftedBufferOriginal);
      }

      return;
    }

    if(usbMessageEndIndex < 0) {
      printf("Error: USB footer is missing (%d,%d) with %d total bytes and %d bytes remaining, attempting to parse anyway...\n", usbMessageStartIndex, usbMessageEndIndex, buffer.dataLength, buffer.dataLength-bufferDataOffset);
      usbMessageEndIndex = buffer.dataLength - 1;
    }

    //atLeastOneHeaderFound = true;

    usbMessageStartIndex += bufferDataOffset;
    usbMessageEndIndex += bufferDataOffset;
    bufferDataOffset = usbMessageEndIndex;

    const s32 messageLength = usbMessageEndIndex-usbMessageStartIndex+1;

    // Move this to a memory-aligned start (index 0 of bigBuffer), after the first MemoryStack::HEADER_LENGTH
    //const s32 bigBuffer_alignedStartIndex = (RoundUp(reinterpret_cast<size_t>(buffer.data), MEMORY_ALIGNMENT) - reinterpret_cast<size_t>(buffer.data)) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH;
    shiftedBuffer = reinterpret_cast<u8*>( RoundUp(reinterpret_cast<size_t>(shiftedBufferOriginal), MEMORY_ALIGNMENT) + MEMORY_ALIGNMENT - MemoryStack::HEADER_LENGTH );
    memcpy(shiftedBuffer, buffer.data+usbMessageStartIndex, messageLength);
    //buffer.data += bigBuffer_alignedStartIndex;

    SerializedBuffer serializedBuffer(shiftedBuffer, messageLength, Anki::Embedded::Flags::Buffer(false, true, true));

    SerializedBufferIterator iterator(serializedBuffer);

    while(iterator.HasNext()) {
      s32 dataLength;
      SerializedBuffer::DataType type;
      u8 * const dataSegmentStart = reinterpret_cast<u8*>(iterator.GetNext(dataLength, type, requireCRCmatch));
      u8 * dataSegment = dataSegmentStart;

      if(!dataSegment) {
        break;
      }

      printf("Next segment is (%d,%d): ", dataLength, type);
      if(type == SerializedBuffer::DATA_TYPE_RAW) {
        printf("Raw segment: ");
        for(s32 i=0; i<dataLength; i++) {
          printf("%x ", dataSegment[i]);
        }
      } else if(type == SerializedBuffer::DATA_TYPE_BASIC_TYPE_BUFFER) {
        SerializedBuffer::EncodedBasicTypeBuffer code;
        for(s32 i=0; i<SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE; i++) {
          code.code[i] = reinterpret_cast<const u32*>(dataSegment)[i];
        }
        dataSegment += SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE * sizeof(u32);
        const s32 remainingDataLength = dataLength - SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE * sizeof(u32);

        u16 size;
        bool isBasicType;
        bool isInteger;
        bool isSigned;
        bool isFloat;
        s32 numElements;
        SerializedBuffer::DecodeBasicTypeBuffer(code, size, isBasicType, isInteger, isSigned, isFloat, numElements);

        printf("Basic type buffer segment (%d, %d, %d, %d, %d): ", size, isInteger, isSigned, isFloat, numElements);
        for(s32 i=0; i<remainingDataLength; i++) {
          printf("%x ", dataSegment[i]);
        }
      } else if(type == SerializedBuffer::DATA_TYPE_ARRAY) {
        SerializedBuffer::EncodedArray code;
        for(s32 i=0; i<SerializedBuffer::EncodedArray::CODE_SIZE; i++) {
          code.code[i] = reinterpret_cast<const u32*>(dataSegment)[i];
        }
        dataSegment += SerializedBuffer::EncodedArray::CODE_SIZE * sizeof(u32);
        const s32 remainingDataLength = dataLength - SerializedBuffer::EncodedArray::CODE_SIZE * sizeof(u32);

        s32 height;
        s32 width;
        s32 stride;
        Flags::Buffer flags;
        u16 basicType_size;
        bool basicType_isBasicType;
        bool basicType_isInteger;
        bool basicType_isSigned;
        bool basicType_isFloat;
        SerializedBuffer::DecodeArrayType(code, height, width, stride, flags, basicType_size, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat);

        printf("Array: (%d, %d, %d, %d, %d, %d, %d, %d) ", height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat);
        //template<typename Type> static Result DeserializeArray(const void * data, const s32 dataLength, Array<Type> &out, MemoryStack &memory);
        if(basicType_size==1 && basicType_isInteger==1 && basicType_isSigned==0 && basicType_isFloat==0) {
          Array<u8> arr;
          SerializedBuffer::DeserializeArray(dataSegmentStart, dataLength, arr, memory);

          snprintf(&outputFilename[0], outputFilenameLength, outputFilenamePattern.data(),
            currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
            currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
            frameNumber,
            "png");

          frameNumber++;

          printf("Saving to %s", outputFilename);
          const cv::Mat_<u8> &mat = arr.get_CvMat_();
          cv::imwrite(outputFilename, mat);
        }
      } else if(type == SerializedBuffer::DATA_TYPE_STRING) {
        printf("Board>> %s", dataSegment);
      } else if(type == SerializedBuffer::DATA_TYPE_CUSTOM) {
        dataSegment[SerializedBuffer::CUSTOM_TYPE_STRING_LENGTH-1] = '\0';
        const char * customTypeName = reinterpret_cast<const char*>(dataSegment);
        //printf(customTypeName);

        dataSegment += SerializedBuffer::CUSTOM_TYPE_STRING_LENGTH;
        s32 remainingDataLength = dataLength - SerializedBuffer::EncodedArray::CODE_SIZE * sizeof(u32);

        if(strcmp(customTypeName, "VisionMarker") == 0) {
          VisionMarker marker;
          marker.Deserialize(reinterpret_cast<void**>(&dataSegment), remainingDataLength);
          marker.Print();
          visionMarkerList.push_back(marker);
          isTracking = false;
        } else if(strcmp(reinterpret_cast<const char*>(customTypeName), "PlanarTransformation_f32") == 0) {
          lastPlanarTransformation.Deserialize(reinterpret_cast<void**>(&dataSegment), remainingDataLength, memory);
          //lastPlanarTransformation.Print();
          isTracking = true;
        }
      }

      printf("\n");
    } // while(iterator.HasNext())
  } // while(bufferDataOffset < buffer.dataLength)

  if(freeBuffer) {
    buffer.data = NULL;
    free(bufferDataOriginal);
    free(shiftedBufferOriginal);
  }

  return;
} // void ProcessRawBuffer()

#endif // #ifndef ROBOT_HARDWARE
