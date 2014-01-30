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

    Result SerializedBuffer::DecodeBasicType(const u32 code, u8 &size, bool &isInteger, bool &isSigned, bool &isFloat)
    {
      if(code & 0xFF)
        isInteger = true;
      else
        isInteger = false;

      if(code & 0xFF00)
        isSigned = true;
      else
        isSigned = false;

      if(code & 0xFF0000)
        isFloat = true;
      else
        isFloat = false;

      size = (code & 0xFF000000) >> 24;

      return RESULT_OK;
    }

    Result SerializedBuffer::DecodeBasicTypeBuffer(const bool swapEndian, const EncodedBasicTypeBuffer &code, u8 &size, bool &isInteger, bool &isSigned, bool &isFloat, s32 &numElements)
    {
      u32 swappedCode[SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE];

      for(s32 i=0; i<SerializedBuffer::EncodedBasicTypeBuffer::CODE_SIZE; i++) {
        if(swapEndian) {
          swappedCode[i] = SwapEndianU32(code.code[i]);
        } else {
          swappedCode[i] = code.code[i];
        }
      }

      SerializedBuffer::DecodeBasicType(swappedCode[0], size, isInteger, isSigned, isFloat);
      numElements = swappedCode[1];

      return RESULT_OK;
    }

    Result SerializedBuffer::DecodeArrayType(const bool swapEndian, const EncodedArray &code, s32 &height, s32 &width, s32 &stride, Flags::Buffer &flags, u8 &basicType_size, bool &basicType_isInteger, bool &basicType_isSigned, bool &basicType_isFloat)
    {
      u32 swappedCode[SerializedBuffer::EncodedArray::CODE_SIZE];

      for(s32 i=0; i<SerializedBuffer::EncodedArray::CODE_SIZE; i++) {
        if(swapEndian) {
          swappedCode[i] = SwapEndianU32(code.code[i]);
        } else {
          swappedCode[i] = code.code[i];
        }
      }

      if(DecodeBasicType(swappedCode[0], basicType_size, basicType_isInteger, basicType_isSigned, basicType_isFloat) != RESULT_OK)
        return RESULT_FAIL;

      height = swappedCode[1];
      width = swappedCode[2];
      stride = swappedCode[3];
      flags.set_rawFlags(swappedCode[4]);

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

    void* SerializedBuffer::PushBack(const void * data, const s32 dataLength)
    {
      return PushBack_Generic(DATA_TYPE_RAW, NULL, 0, data, dataLength);
    }

    void* SerializedBuffer::PushBack(const DataType type, const void * data, s32 dataLength)
    {
      return PushBack_Generic(type, NULL, 0, data, dataLength);
    }

    void* SerializedBuffer::PushBack(const void * header, s32 headerLength, const void * data, s32 dataLength)
    {
      return PushBack_Generic(DATA_TYPE_RAW, header, headerLength, data, dataLength);
    }

    void* SerializedBuffer::PushBack(const DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength)
    {
      return PushBack_Generic(type, header, headerLength, data, dataLength);
    }

    void* SerializedBuffer::PushBack_Generic(const DataType type, const void * header, s32 headerLength, const void * data, s32 dataLength)
    {
      AnkiConditionalErrorAndReturnValue(headerLength >= 0,
        NULL, "SerializedBuffer::PushBack", "headerLength must be >= 0");

      AnkiConditionalErrorAndReturnValue(headerLength%4 == 0,
        NULL, "SerializedBuffer::PushBack", "headerLength must be a multiple of 4");

      if(header != NULL) {
        AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(header)%4 == 0,
          NULL, "SerializedBuffer::PushBack", "header must be 4-byte aligned");
      }

      if(data != NULL) {
        AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(data)%4 == 0,
          NULL, "SerializedBuffer::PushBack", "data must be 4-byte aligned");
      }

      const s32 totalLength = RoundUp<s32>(dataLength, 4) + headerLength;

      const s32 bytesRequired = totalLength+SERIALIZED_SEGEMENT_HEADER_LENGTH+SERIALIZED_SEGMENT_FOOTER_LENGTH;

      s32 numBytesAllocated = -1;
      u8 * const segmentStart = reinterpret_cast<u8*>( memoryStack.Allocate(bytesRequired, numBytesAllocated) );
      u32 * segmentU32 = reinterpret_cast<u32*>(segmentStart);

      AnkiConditionalErrorAndReturnValue(segmentU32 != NULL,
        NULL, "SerializedBuffer::PushBack", "Could not add data");

      u32 crc = 0xFFFFFFFF;

      segmentU32[0] = static_cast<u32>(totalLength);
      segmentU32[1] = static_cast<u32>(type);

      // TODO: decide if the CRC should be computed on the length (png doesn't do this)
#ifdef USING_MOVIDIUS_GCC_COMPILER
      crc =  ComputeCRC32_bigEndian(segmentU32, SERIALIZED_SEGEMENT_HEADER_LENGTH, crc);
#else
      crc =  ComputeCRC32_littleEndian(segmentU32, SERIALIZED_SEGEMENT_HEADER_LENGTH, crc);
#endif

      segmentU32 += (SERIALIZED_SEGEMENT_HEADER_LENGTH>>2);

      if(header != NULL) {
        // Endian-safe copy (it may copy a little extra)

        const s32 headerLength4 = headerLength >> 2;
        const u32 * headerU32 = reinterpret_cast<const u32*>(header);
        for(s32 i=0; i<headerLength4; i++)
        {
          segmentU32[i] = headerU32[i];
        }

#ifdef USING_MOVIDIUS_GCC_COMPILER
        crc =  ComputeCRC32_bigEndian(segmentU32, headerLength, crc);
#else
        crc =  ComputeCRC32_littleEndian(segmentU32, headerLength, crc);
#endif
      } // if(header != NULL)

      segmentU32 += (headerLength>>2);

      if(data != NULL) {
        // Endian-safe copy (it may copy a little extra)

        const s32 dataLength4 = (dataLength + 3) >> 2;
        const u32 * dataU32 = reinterpret_cast<const u32*>(data);

        for(s32 i=0; i<dataLength4; i++)
        {
          segmentU32[i] = dataU32[i];
        }

        const s32 numBytesToCrc = numBytesAllocated-headerLength-SERIALIZED_SEGEMENT_HEADER_LENGTH-SERIALIZED_SEGMENT_FOOTER_LENGTH;
#ifdef USING_MOVIDIUS_GCC_COMPILER
        crc =  ComputeCRC32_bigEndian(segmentU32, numBytesToCrc, crc);
#else
        crc =  ComputeCRC32_littleEndian(segmentU32, numBytesToCrc, crc);
#endif

        // Add a CRC code computed from the header and data
        //const u32 crc2 =  ComputeCRC32_littleEndian(segmentStart, numBytesAllocated - SERIALIZED_SEGMENT_FOOTER_LENGTH, 0xFFFFFFFF);
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

    const void * SerializedBufferConstIterator::GetNext(s32 &dataLength, SerializedBuffer::DataType &type)
    {
      s32 segmentLength = -1;
      const void * segmentToReturn = MemoryStackConstIterator::GetNext(segmentLength);

      AnkiConditionalErrorAndReturnValue(segmentToReturn != NULL,
        NULL, "SerializedBufferConstIterator::GetNext", "segmentToReturn is NULL");

      segmentLength -= SerializedBuffer::SERIALIZED_SEGMENT_FOOTER_LENGTH;

      const u32 expectedCRC = reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(segmentToReturn)+segmentLength)[0];

#ifdef USING_MOVIDIUS_GCC_COMPILER
      const u32 computedCrc =  ComputeCRC32_bigEndian(segmentToReturn, segmentLength, 0xFFFFFFFF);
#else
      const u32 computedCrc =  ComputeCRC32_littleEndian(segmentToReturn, segmentLength, 0xFFFFFFFF);
#endif

      AnkiConditionalErrorAndReturnValue(expectedCRC == computedCrc,
        NULL, "SerializedBufferConstIterator::GetNext", "CRCs don't match (%x != %x)", expectedCRC, computedCrc);

      dataLength = reinterpret_cast<const u32*>(segmentToReturn)[0];
      type = static_cast<SerializedBuffer::DataType>(reinterpret_cast<const u32*>(segmentToReturn)[1]);

      return reinterpret_cast<const u8*>(segmentToReturn) + SerializedBuffer::SERIALIZED_SEGEMENT_HEADER_LENGTH;
    }

    SerializedBufferIterator::SerializedBufferIterator(SerializedBuffer &serializedBuffer)
      : SerializedBufferConstIterator(serializedBuffer)
    {
    }

    void * SerializedBufferIterator::GetNext(const bool swapEndian, s32 &dataLength, SerializedBuffer::DataType &type)
    {
      // To avoid code duplication, we'll use the const version of GetNext(), though our MemoryStack is not const

      u8 * segment = reinterpret_cast<u8*>(const_cast<void*>(SerializedBufferConstIterator::GetNext(dataLength, type)));

      AnkiConditionalErrorAndReturnValue(segment != NULL,
        NULL, "SerializedBufferIterator::GetNext", "segment is NULL");

      if(swapEndian) {
        for(s32 i=0; i<dataLength; i+=4) {
          Swap(segment[i], segment[i^3]);
          Swap(segment[(i+1)], segment[(i+1)^3]);
        }
      }

      return reinterpret_cast<void*>(segment);
    }
  } // namespace Embedded
} // namespace Anki