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

namespace Anki
{
  namespace Embedded
  {
    SerializedBuffer::SerializedBuffer(void *buffer, s32 bufferLength, Flags::Buffer flags)

    {
      if(flags.get_isFullyAllocated()) {
        AnkiConditionalErrorAndReturn((reinterpret_cast<size_t>(buffer)+MemoryStack::HEADER_LENGTH)%MEMORY_ALIGNMENT == 0,
          "SerializedBuffer::SerializedBuffer", "If fully allocated, the %dth byte of the buffer must be %d byte aligned", MemoryStack::HEADER_LENGTH, MEMORY_ALIGNMENT);
      }

      this->memoryStack = MemoryStack(buffer, bufferLength, flags);
    }

    void* SerializedBuffer::PushBack(void * data, const s32 dataLength)
    {
      return PushBack(NULL, 0, data, dataLength);
    }

    void* SerializedBuffer::PushBack(void * header, s32 headerLength, void * data, s32 dataLength)
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

      dataLength = RoundUp<s32>(dataLength, 4);

      s32 numBytesAllocated = -1;
      u8 * const segmentStart = reinterpret_cast<u8*>( memoryStack.Allocate(dataLength+headerLength+SERIALIZED_SEGEMENT_HEADER_LENGTH+SERIALIZED_SEGMENT_FOOTER_LENGTH, numBytesAllocated) );
      u32 * segmentU32 = reinterpret_cast<u32*>(segmentStart);

      AnkiConditionalErrorAndReturnValue(segmentU32 != NULL,
        NULL, "SerializedBuffer::PushBack", "Could not add data");

      u32 crc = 0xFFFFFFFF;

      segmentU32[0] = static_cast<u32>(dataLength);
      segmentU32[1] = static_cast<u32>(DATA_TYPE_RAW);

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
        const u32 * headerU32 = reinterpret_cast<u32*>(header);
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
        const u32 * dataU32 = reinterpret_cast<u32*>(data);
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
      } // if(data != NULL)

      //printf("%d %d\n", reinterpret_cast<u32*>(segmentStart)[0], reinterpret_cast<u32*>(segmentStart)[1]);
      //printf("%d %d %d %d %d %d %d %d\n", segmentStart[0], segmentStart[1], segmentStart[2], segmentStart[3], segmentStart[4], segmentStart[5], segmentStart[6], segmentStart[7]);

      return segmentStart;
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

      segmentLength -= SerializedBuffer::SERIALIZED_SEGMENT_FOOTER_LENGTH;

      const u32 expectedCRC = reinterpret_cast<const u32*>(reinterpret_cast<const u8*>(segmentToReturn)+segmentLength)[0];

#ifdef USING_MOVIDIUS_GCC_COMPILER
      const u32 computedCrc =  ComputeCRC32_bigEndian(segmentToReturn, segmentLength, 0xFFFFFFFF);
#else
      const u32 computedCrc =  ComputeCRC32_littleEndian(segmentToReturn, segmentLength, 0xFFFFFFFF);
#endif

      AnkiConditionalErrorAndReturnValue(expectedCRC == computedCrc,
        NULL, "SerializedBufferConstIterator::GetNext", "CRCs don't match");

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