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

    void* SerializedBuffer::PushBackRaw(const void * data, const s32 dataLength)
    {
      void* dataSegment = reinterpret_cast<u8*>(memoryStack.get_buffer()) + memoryStack.get_usedBytes();

      // Warning: this is dangerous
      memoryStack.usedBytes += dataLength;

      if(data != NULL) {
        memcpy(dataSegment, data, dataLength);
      }

      return dataSegment;
    }

    void* SerializedBuffer::PushBack(const void * data, const s32 dataLength)
    {
      return PushBack_Generic(DATA_TYPE_RAW, NULL, 0, data, dataLength, NULL);
    }

    void* SerializedBuffer::PushBack(const DataType type, const void * data, s32 dataLength)
    {
      return PushBack_Generic(type, NULL, 0, data, dataLength, NULL);
    }

    void* SerializedBuffer::PushBack(const void * header, s32 headerLength, const void * data, s32 dataLength)
    {
      return PushBack_Generic(DATA_TYPE_RAW, header, headerLength, data, dataLength, NULL);
    }

    void* SerializedBuffer::PushBack(const DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength)
    {
      return PushBack_Generic(type, header, headerLength, data, dataLength, NULL);
    }

    void* SerializedBuffer::PushBack(const char * customTypeName, const s32 dataLength, void ** afterHeader)
    {
      return PushBack_Generic(DATA_TYPE_CUSTOM, customTypeName, CUSTOM_TYPE_STRING_LENGTH, NULL, dataLength, afterHeader);
    }

    void* SerializedBuffer::PushBack_Generic(const DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength, void ** afterHeader)
    {
      AnkiConditionalErrorAndReturnValue(headerLength >= 0,
        NULL, "SerializedBuffer::PushBack", "headerLength must be >= 0");

      AnkiConditionalErrorAndReturnValue(headerLength%4 == 0,
        NULL, "SerializedBuffer::PushBack", "headerLength must be a multiple of 4");

      // TODO: are the alignment restrictions still required for the M4?
      //if(header != NULL) {
      //  AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(header)%4 == 0,
      //    NULL, "SerializedBuffer::PushBack", "header must be 4-byte aligned");
      //}

      //if(data != NULL) {
      //  AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(data)%4 == 0,
      //    NULL, "SerializedBuffer::PushBack", "data must be 4-byte aligned");
      //}

      const s32 totalLength = RoundUp<s32>(dataLength, 4) + headerLength;

      const s32 bytesRequired = totalLength + SERIALIZED_SEGMENT_HEADER_LENGTH + SERIALIZED_SEGMENT_FOOTER_LENGTH;

      s32 numBytesAllocated = -1;
      u8 * const segmentStart = reinterpret_cast<u8*>( memoryStack.Allocate(bytesRequired, numBytesAllocated) );
      u32 * segmentU32 = reinterpret_cast<u32*>(segmentStart);

      AnkiConditionalErrorAndReturnValue(segmentU32 != NULL,
        NULL, "SerializedBuffer::PushBack", "Could not add data");

      u32 crc = 0xFFFFFFFF;

      segmentU32[0] = static_cast<u32>(totalLength);
      segmentU32[1] = static_cast<u32>(type);

      // TODO: decide if the CRC should be computed on the length (png doesn't do this)
      crc =  ComputeCRC32(segmentU32, SERIALIZED_SEGMENT_HEADER_LENGTH, crc);

      segmentU32 += (SERIALIZED_SEGMENT_HEADER_LENGTH>>2);

      if(header != NULL) {
        // Endian-safe copy (it may copy a little extra)

        const s32 headerLength4 = headerLength >> 2;
        const u32 * headerU32 = reinterpret_cast<const u32*>(header);
        for(s32 i=0; i<headerLength4; i++)
        {
          segmentU32[i] = headerU32[i];
        }

        crc =  ComputeCRC32(segmentU32, headerLength, crc);
      } // if(header != NULL)

      segmentU32 += (headerLength>>2);

      if(afterHeader != NULL) {
        *afterHeader = reinterpret_cast<void*>(segmentU32);
      }

      if(data != NULL) {
        // Endian-safe copy (it may copy a little extra)

        const s32 dataLength4 = (dataLength + 3) >> 2;
        const u32 * dataU32 = reinterpret_cast<const u32*>(data);

        for(s32 i=0; i<dataLength4; i++)
        {
          segmentU32[i] = dataU32[i];
        }

        const s32 numBytesToCrc = numBytesAllocated-headerLength-SERIALIZED_SEGMENT_HEADER_LENGTH-SERIALIZED_SEGMENT_FOOTER_LENGTH;

        crc =  ComputeCRC32(segmentU32, numBytesToCrc, crc);

        // Add a CRC code computed from the header and data
        //const u32 crc2 =  ComputeCRC32(segmentStart, numBytesAllocated - SERIALIZED_SEGMENT_FOOTER_LENGTH, 0xFFFFFFFF);
        reinterpret_cast<u32*>(segmentStart + numBytesAllocated - SERIALIZED_SEGMENT_FOOTER_LENGTH)[0] = crc;
        //printf("crc: 0x%x\n", crc);
      } // if(data != NULL)

      //printf("%d %d\n", reinterpret_cast<u32*>(segmentStart)[0], reinterpret_cast<u32*>(segmentStart)[1]);
      //printf("%d %d %d %d %d %d %d %d\n", segmentStart[0], segmentStart[1], segmentStart[2], segmentStart[3], segmentStart[4], segmentStart[5], segmentStart[6], segmentStart[7]);

      return segmentStart;
    } // static void* PushBack(const SerializedBuffer::DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength, MemoryStack *scratch)

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

      return PushBack(DATA_TYPE_STRING, &outputString[0], usedLength);
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

      return PushBack(DATA_TYPE_STRING, format, usedLength);
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

    const void * SerializedBufferConstIterator::GetNext(s32 &dataLength, SerializedBuffer::DataType &type, const bool requireFillPatternMatch, const bool requireCRCmatch)
    {
      s32 segmentLength = -1;
      const void * segmentToReturn = MemoryStackConstIterator::GetNext(segmentLength, requireFillPatternMatch);

      AnkiConditionalErrorAndReturnValue(segmentToReturn != NULL,
        NULL, "SerializedBufferConstIterator::GetNext", "segmentToReturn is NULL");

      segmentLength -= SerializedBuffer::SERIALIZED_SEGMENT_FOOTER_LENGTH;

      if(requireCRCmatch) {
        const u32 expectedCRC = reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(segmentToReturn)+segmentLength)[0];

        const u32 computedCrc =  ComputeCRC32(segmentToReturn, segmentLength, 0xFFFFFFFF);

        AnkiConditionalErrorAndReturnValue(expectedCRC == computedCrc,
          NULL, "SerializedBufferConstIterator::GetNext", "CRCs don't match (%x != %x)", expectedCRC, computedCrc);
      } // if(requireCRCmatch)

      dataLength = reinterpret_cast<const u32*>(segmentToReturn)[0];
      type = static_cast<SerializedBuffer::DataType>(reinterpret_cast<const u32*>(segmentToReturn)[1]);

      return reinterpret_cast<const u8*>(segmentToReturn) + SerializedBuffer::SERIALIZED_SEGMENT_HEADER_LENGTH;
    }

    SerializedBufferIterator::SerializedBufferIterator(SerializedBuffer &serializedBuffer)
      : SerializedBufferConstIterator(serializedBuffer)
    {
    }

    void * SerializedBufferIterator::GetNext(s32 &dataLength, SerializedBuffer::DataType &type, const bool requireFillPatternMatch, const bool requireCRCmatch)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const

      u8 * segment = reinterpret_cast<u8*>(const_cast<void*>(SerializedBufferConstIterator::GetNext(dataLength, type, requireFillPatternMatch, requireCRCmatch)));

      //AnkiConditionalErrorAndReturnValue(segment != NULL,
      //  NULL, "SerializedBufferIterator::GetNext", "segment is NULL");

      return reinterpret_cast<void*>(segment);
    }

    SerializedBufferRawConstIterator::SerializedBufferRawConstIterator(const SerializedBuffer &serializedBuffer)
      : MemoryStackRawConstIterator(serializedBuffer.get_memoryStack())
    {
    }

    const void * SerializedBufferRawConstIterator::GetNext(s32 &dataLength, SerializedBuffer::DataType &type, bool &isReportedSegmentLengthCorrect)
    {
      s32 trueSegmentLength;
      s32 reportedSegmentLength;
      const void * segmentToReturn = MemoryStackRawConstIterator::GetNext(trueSegmentLength, reportedSegmentLength);

      if(trueSegmentLength == reportedSegmentLength) {
        isReportedSegmentLengthCorrect = true;
      } else {
        isReportedSegmentLengthCorrect = false;
      }

      AnkiConditionalErrorAndReturnValue(segmentToReturn != NULL,
        NULL, "SerializedBufferConstIterator::GetNext", "segmentToReturn is NULL");

      dataLength = reinterpret_cast<const u32*>(segmentToReturn)[0];
      type = static_cast<SerializedBuffer::DataType>(reinterpret_cast<const u32*>(segmentToReturn)[1]);

      return reinterpret_cast<const u8*>(segmentToReturn) + SerializedBuffer::SERIALIZED_SEGMENT_HEADER_LENGTH;
    }

    SerializedBufferRawIterator::SerializedBufferRawIterator(SerializedBuffer &serializedBuffer)
      : SerializedBufferRawConstIterator(serializedBuffer)
    {
    }

    void * SerializedBufferRawIterator::GetNext(s32 &dataLength, SerializedBuffer::DataType &type, bool &isReportedSegmentLengthCorrect)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const

      u8 * segment = reinterpret_cast<u8*>(const_cast<void*>(SerializedBufferRawConstIterator::GetNext(dataLength, type, isReportedSegmentLengthCorrect)));

      //AnkiConditionalErrorAndReturnValue(segment != NULL,
      //  NULL, "SerializedBufferIterator::GetNext", "segment is NULL");

      return reinterpret_cast<void*>(segment);
    }
  } // namespace Embedded
} // namespace Anki
