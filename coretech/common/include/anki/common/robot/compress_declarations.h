/**
File: compress_declarations.h
Author: Peter Barnum
Created: 2013

Routines for lossless data compression and decompression

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_COMPRESS_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_COMPRESS_DECLARATIONS_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> class CompressedArray;

    //
    // Compression utilities
    //

    // Compress the data "in" into "out"
    // Requires sizeof(heatshrink_encoder) bytes of scratch
    Result Compress(
      const void * in, const u32 inLength,
      void *out, const u32 outMaxLength, s32 &outCompressedLength,
      MemoryStack scratch);

    // Compress the data "in" into "out"
    // Array in must be allocated
    // Requires sizeof(heatshrink_encoder) bytes of scratch
    template<typename Type> Result Compress(
      const Array<Type> &in,
      void *out, const u32 outMaxLength, s32 &outCompressedLength,
      MemoryStack scratch);

    // Compress the data "in" into "out"
    // Array in must be allocated
    // Requires sizeof(heatshrink_encoder) bytes of scratch
    // NOTE: out must be allocated
    template<typename Type> Result Compress(
      const Array<Type> &in,
      CompressedArray<Type> &out,
      MemoryStack scratch);

    //
    // Decompression utilities
    //

    // Decompress the data "in" into "out"
    // Requires sizeof(heatshrink_decoder) bytes of scratch
    Result Decompress(
      const void * in, const u32 inLength,
      void *out, const u32 outMaxLength, s32 &outUncompressedLength,
      MemoryStack scratch);

    // Decompress the data "in" into a returned Array
    // Requires sizeof(heatshrink_decoder) bytes of scratch
    // Permanently allocates the Array as well
    template<typename Type> Array<Type> Decompress(
      const void * in, const u32 inLength,
      const s32 arrayHeight, const s32 arrayWidth, const Flags::Buffer &arrayFlags,
      MemoryStack &memory);

    // Decompress the data "in" into a returned Array
    // Requires sizeof(heatshrink_decoder) bytes of scratch
    // Permanently allocates the Array as well
    template<typename Type> Array<Type> Decompress(
      CompressedArray<Type> &in,
      MemoryStack &memory);

    // A CompressedArray just holds the compresed data for an Array, it doesn't modify it, or
    // compress/decompress it. This is a little weird, but it is tricky to save memory with
    // compression without using a heap.
    template<typename Type> class CompressedArray
    {
    public:
      CompressedArray();

      // compressedBuffer must be allocated with at least compressedBufferMaxLength bytes
      // if compressedBuffer doesn't contains data, compressedBufferUsedLength is zero
      CompressedArray(
        void * compressedBuffer, const s32 compressedBufferMaxLength, const s32 compressedBufferUsedLength,
        const s32 arrayHeight, const s32 arrayWidth, const Flags::Buffer &arrayFlags);

      bool IsValid() const;

      s32 get_arraySize(s32 dimension) const;
      Flags::Buffer get_arrayFlags() const;

      const void* get_compressedBuffer() const;
      void* get_compressedBuffer();

      s32 get_compressedBufferMaxLength() const;
      s32 get_compressedBufferUsedLength() const;

      void set_compressedBufferUsedLength(const s32 compressedBufferUsedLength);

    protected:
      void * compressedBuffer;
      s32 compressedBufferMaxLength;
      s32 compressedBufferUsedLength;

      // Original Array parameters
      s32 arraySize[2];
      Flags::Buffer arrayFlags;
    }; // template<typename Type> class CompressedArray
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPRESS_DECLARATIONS_H_
