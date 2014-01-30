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
//#include "anki/cozmo/robot/messages.h"
#include "anki/common/robot/serialize.h"

#include <ctime>

using namespace Anki::Embedded;
using namespace std;

#ifndef ROBOT_HARDWARE

#ifndef _MSC_VER
#error Currently, only visual c++ is supported
#endif

void FindSerializedBuffer(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex)
{
  const u8 * rawBufferU8 = reinterpret_cast<const u8*>(rawBuffer);

  s32 state = 0;
  s32 index = 0;

  startIndex = -1;
  endIndex = -1;

  // Look for the header
  while(index < rawBufferLength) {
    if(state == SERIALIZED_BUFFER_HEADER_LENGTH) {
      startIndex = index;
      break;
    }

    if(rawBufferU8[index] == SERIALIZED_BUFFER_HEADER[state]) {
      state++;
    } else if(rawBufferU8[index] == SERIALIZED_BUFFER_HEADER[0]) {
      state = 1;
    } else {
      state = 0;
    }

    index++;
  } // while(index < rawBufferLength)

  // Look for the footer
  state = 0;
  while(index < rawBufferLength) {
    if(state == SERIALIZED_BUFFER_FOOTER_LENGTH) {
      endIndex = index-SERIALIZED_BUFFER_FOOTER_LENGTH-1;
      break;
    }

    //printf("%d) %d %x %x\n", index, state, rawBufferU8[index], SERIALIZED_BUFFER_FOOTER[state]);

    if(rawBufferU8[index] == SERIALIZED_BUFFER_FOOTER[state]) {
      state++;
    } else if(rawBufferU8[index] == SERIALIZED_BUFFER_FOOTER[0]) {
      state = 1;
    } else {
      state = 0;
    }

    index++;
  } // while(index < rawBufferLength)

  if(state == SERIALIZED_BUFFER_FOOTER_LENGTH) {
    endIndex = index-SERIALIZED_BUFFER_FOOTER_LENGTH-1;
  }
} // void FindSerializedBuffer(const void * rawBuffer, s32 &startIndex, s32 &endIndex)

void ProcessRawBuffer(RawBuffer &buffer, const string outputFilenamePattern, const bool freeBuffer, const BufferAction action)
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
    FindSerializedBuffer(buffer.data+bufferDataOffset, buffer.dataLength-bufferDataOffset, usbMessageStartIndex, usbMessageEndIndex);

    if(usbMessageStartIndex < 0) {
      printf("Error: USB header is missing (%d,%d) with %d total bytes and %d bytes remaining, returning...\n", usbMessageStartIndex, usbMessageEndIndex, buffer.dataLength, buffer.dataLength-bufferDataOffset);

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

    if(swapEndianForHeaders) {
      for(s32 i=0; i<messageLength; i+=4) {
        Swap(shiftedBuffer[i], shiftedBuffer[i^3]);
        Swap(shiftedBuffer[(i+1)], shiftedBuffer[(i+1)^3]);
      }
    }

    SerializedBuffer serializedBuffer(shiftedBuffer, messageLength, Anki::Embedded::Flags::Buffer(false, true, true));

    SerializedBufferIterator iterator(serializedBuffer);

    while(iterator.HasNext()) {
      s32 dataLength;
      SerializedBuffer::DataType type;
      u8 * const dataSegmentStart = reinterpret_cast<u8*>(iterator.GetNext(swapEndianForHeaders, dataLength, type));
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

        u8 size;
        bool isInteger;
        bool isSigned;
        bool isFloat;
        s32 numElements;
        SerializedBuffer::DecodeBasicTypeBuffer(swapEndianForHeaders, code, size, isInteger, isSigned, isFloat, numElements);

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
        u8 basicType_size;
        bool basicType_isInteger;
        bool basicType_isSigned;
        bool basicType_isFloat;
        SerializedBuffer::DecodeArrayType(swapEndianForHeaders, code, height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat);

        printf("Array: (%d, %d, %d, %d, %d, %d, %d, %d) ", height, width, stride, flags, basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat);
        //template<typename Type> static Result DeserializeArray(const void * data, const s32 dataLength, Array<Type> &out, MemoryStack &memory);
        if(basicType_size==1 && basicType_isInteger==1 && basicType_isSigned==0 && basicType_isFloat==0) {
          Array<u8> arr;
          SerializedBuffer::DeserializeArray(swapEndianForHeaders, swapEndianForContents, dataSegmentStart, dataLength, arr, memory);

          snprintf(&outputFilename[0], outputFilenameLength, outputFilenamePattern.data(),
            currentTime->tm_year+1900, currentTime->tm_mon+1, currentTime->tm_mday,
            currentTime->tm_hour, currentTime->tm_min, currentTime->tm_sec,
            frameNumber,
            "png");

          frameNumber++;

          printf("Saving to %s", outputFilename);
          const cv::Mat_<u8> &mat = arr.get_CvMat_();
          cv::imwrite(outputFilename, mat);
          //cv::imwrite("c:/tmp/tt.bmp", mat);
        }
      } else if(type == SerializedBuffer::DATA_TYPE_STRING) {
        printf("Board>> %s", dataSegment);
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