/**
File: serialize.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/serialize.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/compress.h"

namespace Anki
{
  namespace Embedded
  {
    SerializedBuffer::SerializedBuffer()
    {
      this->memoryStack = MemoryStack();
    }

    SerializedBuffer::SerializedBuffer(void *buffer, s32 bufferLength, Flags::Buffer flags)

    {
      if(flags.get_isFullyAllocated()) {
        AnkiConditionalErrorAndReturn((reinterpret_cast<size_t>(buffer)+MemoryStack::HEADER_LENGTH)%MEMORY_ALIGNMENT == 0,
          "SerializedBuffer::SerializedBuffer", "If fully allocated, the %dth byte of the buffer must be %d byte aligned", MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT);
      }

      this->memoryStack = MemoryStack(buffer, bufferLength, flags);
    }

    Result SerializedBuffer::DecodeBasicType(const u32 code, u16 &size, bool &isBasicType, bool &isInteger, bool &isSigned, bool &isFloat)
    {
      if(code & 1)
        isBasicType = true;
      else
        isBasicType = false;

      if(code & 2)
        isInteger = true;
      else
        isInteger = false;

      if(code & 4)
        isSigned = true;
      else
        isSigned = false;

      if(code & 8)
        isFloat = true;
      else
        isFloat = false;

      size = (code & 0xFFFF0000) >> 16;

      return RESULT_OK;
    }

    Result SerializedBuffer::DecodeBasicTypeBuffer(const EncodedBasicTypeBuffer &code, u16 &size, bool &isBasicType, bool &isInteger, bool &isSigned, bool &isFloat, s32 &numElements)
    {
      SerializedBuffer::DecodeBasicType(code.code[0], size, isBasicType, isInteger, isSigned, isFloat);
      numElements = code.code[1];

      return RESULT_OK;
    }

    Result SerializedBuffer::DecodeArrayType(const EncodedArray &code, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, u16 &basicType_size, bool &basicType_isBasicType, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat)
    {
      if(DecodeBasicType(code.code[0], basicType_size, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat) != RESULT_OK)
        return RESULT_FAIL;

      height = code.code[1];
      width = code.code[2];
      stride = code.code[3];
      flags.set_rawFlags(code.code[4]);

      return RESULT_OK;
    }

    Result SerializedBuffer::DecodeArraySliceType(const EncodedArraySlice &code, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, s32 &ySlice_start, s32 &ySlice_increment, s32 &ySlice_end, s32 &xSlice_start, s32 &xSlice_increment, s32 &xSlice_end, u16 &basicType_size, bool &basicType_isBasicType, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat)
    {
      // The first part of the code is the same as an Array
      EncodedArray arrayCode;

      for(s32 i=0; i<5; i++) {
        arrayCode.code[i] = code.code[i];
      }

      const Result result = SerializedBuffer::DecodeArrayType(arrayCode, height, width, stride, flags, basicType_size, basicType_isBasicType, basicType_isInteger, basicType_isSigned, basicType_isFloat);

      if(result != RESULT_OK)
        return result;

      ySlice_start = *reinterpret_cast<const s32*>(&code.code[5]);
      ySlice_increment = *reinterpret_cast<const s32*>(&code.code[6]);
      ySlice_end = *reinterpret_cast<const s32*>(&code.code[7]);
      xSlice_start = *reinterpret_cast<const s32*>(&code.code[8]);
      xSlice_increment = *reinterpret_cast<const s32*>(&code.code[9]);
      xSlice_end = *reinterpret_cast<const s32*>(&code.code[10]);

      return RESULT_OK;
    }

    Result SerializedBuffer::FindSerializedBuffer(const void * rawBuffer, const s32 rawBufferLength, s32 &startIndex, s32 &endIndex)
    {
      startIndex = -1;
      endIndex = -1;

      AnkiConditionalErrorAndReturnValue(rawBuffer != NULL,
        RESULT_FAIL_UNINITIALIZED_MEMORY, "SerializedBuffer::FindSerializedBuffer", "rawBuffer is NULL");

      AnkiConditionalErrorAndReturnValue(rawBufferLength >= 0,
        RESULT_FAIL_INVALID_PARAMETERS, "SerializedBuffer::FindSerializedBuffer", "rawBufferLength is >= 0");

      if(rawBufferLength == 0)
        return RESULT_OK;

      const u8 * rawBufferU8 = reinterpret_cast<const u8*>(rawBuffer);

      s32 state = 0;
      s32 index = 0;

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

      return RESULT_OK;
    } // void FindSerializedBuffer(const void * rawBuffer, s32 &startIndex, s32 &endIndex)

    Result SerializedBuffer::SerializeDescriptionString(const char *objectName, void ** buffer, s32 &bufferLength)
    {
      if(bufferLength < DESCRIPTION_STRING_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      u8 * bufferU8 = *reinterpret_cast<u8**>(buffer);

      s32 iChar = 0;

      // If objectName is not NULL, copy it
      if(objectName) {
        for(iChar=0; iChar<(DESCRIPTION_STRING_LENGTH-1); iChar++) {
          if(objectName[iChar] == '\0') {
            break;
          }

          bufferU8[iChar] = objectName[iChar];
        }
      } // if(objectName)

      bufferU8[iChar] = '\0';

      *reinterpret_cast<u8**>(buffer) += DESCRIPTION_STRING_LENGTH;
      bufferLength -= DESCRIPTION_STRING_LENGTH;

      return RESULT_OK;
    }

    Result SerializedBuffer::DeserializeDescriptionString(char *objectName, void ** buffer, s32 &bufferLength)
    {
      if(bufferLength < DESCRIPTION_STRING_LENGTH) {
        return RESULT_FAIL_OUT_OF_MEMORY;
      }

      if(objectName) {
        u8 * bufferU8 = *reinterpret_cast<u8**>(buffer);
        s32 iChar = 0;

        for(iChar=0; iChar<(DESCRIPTION_STRING_LENGTH-1); iChar++) {
          if(bufferU8[iChar] == '\0') {
            break;
          }

          objectName[iChar] = bufferU8[iChar];
        }

        objectName[iChar] = '\0';
      }

      *reinterpret_cast<u8**>(buffer) += DESCRIPTION_STRING_LENGTH;
      bufferLength -= DESCRIPTION_STRING_LENGTH;

      return RESULT_OK;
    }

    void* SerializedBuffer::AllocateRaw(const s32 dataLength)
    {
      const s32 bytesRequired = RoundUp<s32>(dataLength, 4);

      s32 numBytesAllocated;

      u8 * segment = reinterpret_cast<u8*>( memoryStack.Allocate(bytesRequired, numBytesAllocated) );

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::AllocateRaw", "Could not add data");

      return segment;
    } // static void* PushBack(const SerializedBuffer::DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength, MemoryStack *scratch)

    void* SerializedBuffer::Allocate(const char *typeName, const char *objectName, const s32 dataLength)
    {
      s32 totalLength = dataLength + 2*DESCRIPTION_STRING_LENGTH;
      void *segment = AllocateRaw(totalLength);

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::Allocate", "Could not add data");

      if(SerializeDescriptionString(typeName, reinterpret_cast<void**>(&segment), totalLength) != RESULT_OK)
        return NULL;

      if(SerializeDescriptionString(objectName, reinterpret_cast<void**>(&segment), totalLength) != RESULT_OK)
        return NULL;

      return segment;
    }

    void* SerializedBuffer::PushBackString(const char * format, ...)
    {
      // After switching to a better board, this will work
#if 0
      const s32 outputStringLength = 1024;
      static char outputString[outputStringLength];

      va_list arguments;
      va_start(arguments, format);
      snprintf(outputString, outputStringLength, format, arguments);
      va_end(arguments);

      s32 usedLength = outputStringLength;
      for(s32 i=0; i<outputStringLength; i++) {
        if(outputString[i] == '\0') {
          usedLength = i;
          break;
        }
      }

      if(usedLength == outputStringLength) {
        outputString[outputStringLength-1] = '\0';
      }

      return PushBack("String", DATA_TYPE_STRING, &outputString[0], usedLength);
#else
      // Temporary hack

      s32 usedLength = -1;
      for(s32 i=0; i<1024; i++) {
        if(format[i] == '\0') {
          usedLength = i;
          break;
        }
      }

      if(usedLength == -1)
        return NULL;

      void* segment = Allocate("String", "String", usedLength);

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBuffer::PushBackString", "Could not add data");

      memcpy(segment, format, usedLength);

      return segment;
#endif
    }

    bool SerializedBuffer::IsValid() const
    {
      return memoryStack.IsValid();
    }

    const MemoryStack& SerializedBuffer::get_memoryStack() const
    {
      return memoryStack;
    }

    MemoryStack& SerializedBuffer::get_memoryStack()
    {
      return memoryStack;
    }

    SerializedBufferConstIterator::SerializedBufferConstIterator(const SerializedBuffer &serializedBuffer)
      : MemoryStackConstIterator(serializedBuffer.get_memoryStack())
    {
    }

    const void * SerializedBufferConstIterator::GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, const bool requireFillPatternMatch)
    {
      dataLength = -1;
      const char * segment = reinterpret_cast<const char*>( MemoryStackConstIterator::GetNext(dataLength, requireFillPatternMatch) );

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBufferConstIterator::GetNext", "segmentToReturn is NULL");

      *typeName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;
      dataLength -= SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      *objectName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;
      dataLength -= SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      return segment;
    }

    SerializedBufferIterator::SerializedBufferIterator(SerializedBuffer &serializedBuffer)
      : SerializedBufferConstIterator(serializedBuffer)
    {
    }

    void * SerializedBufferIterator::GetNext(const char **typeName, const char **objectName, s32 &dataLength, const bool requireFillPatternMatch)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const
      void * segment = const_cast<void*>(SerializedBufferConstIterator::GetNext(typeName, objectName, dataLength, requireFillPatternMatch));

      return segment;
    }

    SerializedBufferReconstructingConstIterator::SerializedBufferReconstructingConstIterator(const SerializedBuffer &serializedBuffer)
      : MemoryStackReconstructingConstIterator(serializedBuffer.get_memoryStack())
    {
    }

    const void * SerializedBufferReconstructingConstIterator::GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, bool &isReportedSegmentLengthCorrect)
    {
      s32 trueSegmentLength;
      s32 reportedSegmentLength;
      const char * segment = reinterpret_cast<const char*>( MemoryStackReconstructingConstIterator::GetNext(trueSegmentLength, reportedSegmentLength) );

      if(trueSegmentLength == reportedSegmentLength) {
        isReportedSegmentLengthCorrect = true;
      } else {
        isReportedSegmentLengthCorrect = false;
      }

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBufferConstIterator::GetNext", "segmentToReturn is NULL");

      *typeName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      *objectName = segment;
      segment += SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      dataLength = reinterpret_cast<const u32*>(segment)[0];
      segment += sizeof(u32);

      return segment;
    }

    SerializedBufferReconstructingIterator::SerializedBufferReconstructingIterator(SerializedBuffer &serializedBuffer)
      : SerializedBufferReconstructingConstIterator(serializedBuffer)
    {
    }

    void * SerializedBufferReconstructingIterator::GetNext(const char ** typeName, const char ** objectName, s32 &dataLength, bool &isReportedSegmentLengthCorrect)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const
      void * segment = const_cast<void*>(SerializedBufferReconstructingConstIterator::GetNext(typeName, objectName, dataLength, isReportedSegmentLengthCorrect));

      return segment;
    }
  } // namespace Embedded
} // namespace Anki
