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
      : MemoryStack(buffer, bufferLength, flags)
    {
    }

    void* SerializedBuffer::PushBack(void * data, const s32 dataLength)
    {
      return PushBack(NULL, 0, data, dataLength);
    }

    void* SerializedBuffer::PushBack(void * header, s32 headerLength, void * data, s32 dataLength)
    {
      if(header != NULL) {
        AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(header)%4 == 0,
          NULL, "SerializedBuffer::PushBack", "header must be 4-byte aligned");
      }

      AnkiConditionalErrorAndReturnValue(headerLength >= 0,
        NULL, "SerializedBuffer::PushBack", "headerLength must be >= 0");

      AnkiConditionalErrorAndReturnValue(headerLength%4 == 0,
        NULL, "SerializedBuffer::PushBack", "headerLength must be a multiple of 4");

      if(data != NULL) {
        AnkiConditionalErrorAndReturnValue(reinterpret_cast<size_t>(data)%4 == 0,
          NULL, "SerializedBuffer::PushBack", "data must be 4-byte aligned");
      }

      dataLength = RoundUp(dataLength, 4);

      u32* segmentU32 = reinterpret_cast<u32*>( MemoryStack::Allocate(dataLength+headerLength+SERIALIZED_HEADER_LENGTH) );

      AnkiConditionalErrorAndReturnValue(segmentU32 != NULL,
        NULL, "SerializedBuffer::PushBack", "Could not add data");

      segmentU32[0] = static_cast<u32>(dataLength);
      segmentU32[1] = static_cast<u32>(DATA_TYPE_RAW);
      segmentU32 += (SERIALIZED_HEADER_LENGTH>>2);

      //
      // Endian-safe copy (it may copy a little extra)
      //

      if(header != NULL) {
        const s32 headerLength4 = headerLength >> 2;
        const u32 * headerU32 = reinterpret_cast<u32*>(header);
        for(s32 i=0; i<headerLength4; i++)
        {
          segmentU32[i] = headerU32[i];
        }
      } // if(data != NULL)

      segmentU32 += (headerLength>>2);

      if(data != NULL) {
        const s32 dataLength4 = (dataLength + 3) >> 2;
        const u32 * dataU32 = reinterpret_cast<u32*>(data);
        for(s32 i=0; i<dataLength4; i++)
        {
          segmentU32[i] = dataU32[i];
        }
      } // if(data != NULL)

      return segmentU32;
    }
  } // namespace Embedded
} // namespace Anki