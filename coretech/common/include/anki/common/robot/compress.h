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
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> Result Compress(
      const Array<Type> &in,
      void *out, const u32 outMaxLength, s32 &outCompressedLength,
      MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Compress", "Array in is not valid");

      AnkiConditionalErrorAndReturnValue(out != NULL,
        RESULT_FAIL_UNINITIALIZED_MEMORY, "Compress", "out is NULL");

      return Compress(in.Pointer(0,0), in.get_size(0)*in.get_stride(),
        out, outMaxLength, outCompressedLength,
        scratch);
    }

    template<typename Type> Result Compress(
      const Array<Type> &in,
      CompressedArray<Type> &out,
      MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(in.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Compress", "Array in is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Compress", "CompressedArray out is not valid");

      s32 outCompressedLength;
      const Result result = Compress(in.Pointer(0,0), in.get_size(0)*in.get_stride(),
        out.get_compressedBuffer(), out.get_compressedBufferMaxLength(), outCompressedLength,
        scratch);

      out.set_compressedBufferUsedLength(outCompressedLength);

      return result;
    }

    template<typename Type> Array<Type> Decompress(
      const void * in, const u32 inLength,
      const s32 arrayHeight, const s32 arrayWidth, const Flags::Buffer &arrayFlags,
      MemoryStack &memory)
    {
      Array<Type> out = Array<Type>();

      const s32 outSize = Array<Type>::ComputeMinimumRequiredMemory(arrayHeight, arrayWidth, arrayFlags);
      const s32 bufferSize = outSize + (outSize >> 1) + 4;

      void * uncompressedData = memory.Allocate(bufferSize);

      s32 outUncompressedLength;
      const Result result1 = Decompress(
        in, inLength,
        uncompressedData, bufferSize, outUncompressedLength,
        memory);

      AnkiConditionalErrorAndReturnValue(result1 == RESULT_OK,
        out, "Decompress", "Decompress subcall failed");

      s32 numBytesAllocated;
      uncompressedData = memory.Reallocate(uncompressedData, outSize, numBytesAllocated);

      AnkiConditionalErrorAndReturnValue(uncompressedData != NULL,
        out, "Decompress", "Reallocate failed");

      Flags::Buffer correctedFlags = arrayFlags;
      correctedFlags.set_isFullyAllocated(false);
      correctedFlags.set_zeroAllocatedMemory(false);

      out = Array<Type>(arrayHeight, arrayWidth,
        uncompressedData, numBytesAllocated, correctedFlags);

      return out;
    }

    template<typename Type> Array<Type> Decompress(
      CompressedArray<Type> &in,
      MemoryStack &memory)
    {
      return Decompress<Type>(
        in.get_compressedBuffer(), in.get_compressedBufferUsedLength(),
        in.get_arraySize(0), in.get_arraySize(1), in.get_arrayFlags(),
        memory);
    }

    template<typename Type> CompressedArray<Type>::CompressedArray()
      : compressedBuffer(NULL), compressedBufferMaxLength(-1), compressedBufferUsedLength(-1)
    {
      this->arraySize[0] = -1;
      this->arraySize[0] = -1;
    }

    template<typename Type> CompressedArray<Type>::CompressedArray(void * compressedBuffer, const s32 compressedBufferMaxLength, const s32 compressedBufferUsedLength, const s32 arrayHeight, const s32 arrayWidth, const Flags::Buffer &arrayFlags)
      : compressedBuffer(compressedBuffer), compressedBufferMaxLength(compressedBufferMaxLength), compressedBufferUsedLength(compressedBufferUsedLength), arrayFlags(arrayFlags)
    {
      this->arraySize[0] = arrayHeight;
      this->arraySize[1] = arrayWidth;
    }

    template<typename Type> bool CompressedArray<Type>::IsValid() const
    {
      if(compressedBuffer == NULL)
        return false;

      if(compressedBufferMaxLength < 0 || compressedBufferUsedLength > compressedBufferMaxLength)
        return false;

      if(arraySize[0] < 0 || arraySize[1] < 0)
        return false;

      return true;
    }

    template<typename Type> s32 CompressedArray<Type>::get_arraySize(s32 dimension) const
    {
      AnkiConditionalErrorAndReturnValue(dimension >= 0,
        0, "CompressedArray<Type>::get_size", "Negative dimension");

      if(dimension > 1 || dimension < 0)
        return 1;

      return arraySize[dimension];
    }

    template<typename Type> Flags::Buffer CompressedArray<Type>::get_arrayFlags() const
    {
      return arrayFlags;
    }

    template<typename Type> const void* CompressedArray<Type>::get_compressedBuffer() const
    {
      return compressedBuffer;
    }

    template<typename Type> void* CompressedArray<Type>::get_compressedBuffer()
    {
      return compressedBuffer;
    }

    template<typename Type> s32 CompressedArray<Type>::get_compressedBufferMaxLength() const
    {
      return compressedBufferMaxLength;
    }

    template<typename Type> s32 CompressedArray<Type>::get_compressedBufferUsedLength() const
    {
      return compressedBufferUsedLength;
    }

    template<typename Type> void CompressedArray<Type>::set_compressedBufferUsedLength(const s32 compressedBufferUsedLength)
    {
      this->compressedBufferUsedLength = compressedBufferUsedLength;
    }
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_
