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

#include "anki/common/robot/config.h"
#include "anki/common/robot/memory.h"

namespace Anki
{
  namespace Embedded
  {
    // Compress the data "in" into "out"
    // Requires sizeof(heatshrink_encoder) bytes of scratch
    Result Compress(const u8 * in, const u32 inLength, u8 *out, const u32 outMaxLength, s32 &outCompressedLength, MemoryStack scratch);

    // Decompress the data "in" into "out"
    // Requires sizeof(heatshrink_decoder) bytes of scratch
    Result Decompress(const u8 * in, const u32 inLength, u8 *out, const u32 outMaxLength, s32 &outUncompressedLength, MemoryStack scratch);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_COMPRESS_H_
