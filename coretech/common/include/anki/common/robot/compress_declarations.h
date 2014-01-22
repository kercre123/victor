/**
File: compress_declarations.h
Author: Peter Barnum
Created: 2013

Routines for lossless data compression and decompression

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_
#define _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/memory.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class CompressedArray
    {
    public:
      CompressedArray();

      // Requires sizeof(heatshrink_encoder)+sizeof(Array) bytes of scratch
      CompressedArray(const Array<Type> &in, MemoryStack &memory);

      // Requires the maximum size of the array + sizeof(heatshrink_decoder) bytes of scratch
      // Permanently allocates the Array as well
      Array<Type> Decompress(MemoryStack &memory) const;

      // Similar to Matlabs size(matrix, dimension), and dimension is in {0,1}
      s32 get_size(s32 dimension) const;

    protected:
      Array<Type> data;

      s32 size[2];
      Flags::Buffer flags;
    }; // template<typename Type> class CompressedArray

    // Compress the data "in" into "out"
    // Requires sizeof(heatshrink_encoder) bytes of scratch
    Result Compress(const void * in, const u32 inLength, void *out, const u32 outMaxLength, s32 &outCompressedLength, MemoryStack scratch);

    // Decompress the data "in" into "out"
    // Requires sizeof(heatshrink_decoder) bytes of scratch
    Result Decompress(const void * in, const u32 inLength, void *out, const u32 outMaxLength, s32 &outUncompressedLength, MemoryStack scratch);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_
