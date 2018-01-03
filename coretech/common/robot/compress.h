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

#include "coretech/common/robot/compress_declarations.h"
#include "coretech/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    template<typename Type> Result EncodeRunLengthBinary(
      const Array<Type> &in,
      u8 * restrict out, const s32 outMaxLength, s32 &outCompressedLength)
    {
      return EncodeRunLengthBinary(in.Pointer(0,0), in.get_size(0)*in.get_stride(),
        out, outMaxLength, outCompressedLength);
    }

    template<typename Type> Array<Type> DecodeRunLengthBinary(
      const u8 * restrict in, const s32 inLength,
      const s32 arrayHeight, const s32 arrayWidth, const Flags::Buffer &arrayFlags,
      MemoryStack &memory)
    {
      Flags::Buffer correctedFlags = arrayFlags;
      correctedFlags.set_zeroAllocatedMemory(false);

      Array<Type> out = Array<Type>(arrayHeight, arrayWidth, memory, correctedFlags);

      s32 outUncompressedLength;

      const Result result = DecodeRunLengthBinary(
        in, inLength,
        reinterpret_cast<u8*>(out.Pointer(0,0)), out.get_size(0)*out.get_stride(), outUncompressedLength);

      AnkiConditionalErrorAndReturnValue(result == RESULT_OK,
        Array<Type>(), "DecodeRunLengthBinary", "DecodeRunLengthBinary failed");

      return out;
    }
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_
