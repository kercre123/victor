/**
File: sequences_declarations.h
Author: Peter Barnum
Created: 2013

A Sequence is a mathematically-defined, ordered list. The sequence classes allow for operations on sequences, without requiring them to be explicitly evaluated.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_
#define _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/flags.h"
#include "anki/common/robot/memory.h"

namespace Anki
{
  namespace Embedded
  {
    class SerializedBuffer
    {
    public:
      SerializedBuffer() : buffer() { }
      SerializedBuffer(void *buffer, const s32 bufferLength, const Flags::Buffer flags=Flags::Buffer(true,true));

    protected:
      MemoryStack buffer;
    }; // class SerializedBuffer

    class SerializedBufferIterator
    {
    public:

    protected:
    }; // class SerializedBufferIterator
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SERIALIZE_H_