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

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    // Encodes the input binary buffer in with run length encoding. Each non-zero byte is treated as one, each zero byte as zero.
    //
    // The output length will always be less-than-or-equal to the input length
    //
    // The encoded format is:
    // bit 0 (MSB): If 1, the segment is all ones.
    // bits 1-7: Unsigned length of the segment minus 1 (possible values from 1-128)
    Result EncodeRunLengthBinary(
      const u8 * restrict in, const s32 inLength,
      u8 * restrict out, const s32 outMaxLength, s32 &outCompressedLength);

    template<typename Type> Result EncodeRunLengthBinary(
      const Array<Type> &in,
      u8 * restrict out, const s32 outMaxLength, s32 &outCompressedLength);

    Result DecodeRunLengthBinary(
      const u8 * restrict in, const s32 inLength,
      u8 * restrict out, const s32 outMaxLength, s32 &outUncompressedLength);

    template<typename Type> Array<Type> DecodeRunLengthBinary(
      const u8 * restrict in, const s32 inLength,
      const s32 arrayHeight, const s32 arrayWidth, const Flags::Buffer &arrayFlags,
      MemoryStack &memory);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPRESS_DECLARATIONS_H_
