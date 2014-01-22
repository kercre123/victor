/**
File: compress.h
Author: Peter Barnum
Created: 2013

Routines for lossless data compression and decompression

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_
#define _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_

#include "anki/common/robot/compress_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> CompressedArray::CompressedArray()
      : data()
    {
      this->size[0] = -1;
      this->size[0] = -1;
    }

    template<typename Type> CompressedArray::CompressedArray(const Array<Type> &in, MemoryStack &memory)
    {
      const s32 inSize = in.get_stride() * in.get_size(0);
      const s32 bufferSize = inSize + inSize >> 1 + 4;

      data = Array<Type>(1, bufferSize, memory);

      s32 outCompressedLength;
      Compress(in.Pointer(0,0), inSize, data.Pointer(0,0), bufferSize, outCompressedLength, memory);

      // TODO: resize in to its exact size
    }

    template<typename Type> Array<Type> CompressedArray::Decompress(MemoryStack &memory) const
    {
    }

    template<typename Type> s32 CompressedArray::get_size(s32 dimension) const
    {
    }
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_
