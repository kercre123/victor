/**
File: compress.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/common/robot/compress.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/errorHandling.h"

namespace Anki
{
  namespace Embedded
  {
    Result EncodeRunLengthBinary(
      const u8 * restrict in, const s32 inLength,
      u8 * restrict out, const s32 outMaxLength, s32 &outCompressedLength)
    {
      s32 curComponentCount = 0;
      s32 curComponentColor;

      if(in[0] == 0) {
        curComponentColor = 0;
      } else {
        curComponentColor = 1;
      }

      outCompressedLength = 0;

      for(s32 iByte=1; iByte<inLength; iByte++) {
        // If the current component is the same color as the current pixel,
        // and not too big, keep growing it

        const bool isNewColor = (curComponentColor == 0 && in[iByte] != 0) || (curComponentColor == 1 && in[iByte] == 0);

        if(isNewColor || curComponentCount > 126) {
          out[outCompressedLength] = static_cast<u8>(curComponentColor * 128 + curComponentCount);
          outCompressedLength++;

          if(outCompressedLength >= (outMaxLength-1)) {
            return RESULT_FAIL_OUT_OF_MEMORY;
          }

          curComponentCount = 0; // Actually 1, but we're storing (count-1)

          if(in[iByte] == 0) {
            curComponentColor = 0;
          } else {
            curComponentColor = 1;
          }
        } else {
          curComponentCount++;
        }
      } // for(s32 iByte=1; iByte<inLength; iByte++)

      out[outCompressedLength] = static_cast<u8>(curComponentColor * 128 + curComponentCount);
      outCompressedLength++;

      return RESULT_OK;
    } // EncodeRunLengthBinary()

    Result DecodeRunLengthBinary(
      const u8 * restrict in, const s32 inLength,
      u8 * restrict out, const s32 outMaxLength, s32 &outUncompressedLength)
    {
      outUncompressedLength = 0;

      for(s32 iIn=0; iIn<inLength; iIn++) {
        const u8 curByte = in[iIn];

        const u8 curComponentColor = (curByte & 128) >> 7;
        const s32 curComponentCount = curByte & 127;

        if((curComponentCount + outUncompressedLength) >= outMaxLength) {
          return RESULT_FAIL_OUT_OF_MEMORY;
        }

        // Note: curComponentCount is (count-1)
        for(s32 iOut=0; iOut<=curComponentCount; iOut++) {
          out[iOut + outUncompressedLength] = curComponentColor;
        }

        outUncompressedLength += curComponentCount + 1;
      } // for(s32 iByte=0; iByte<inLength; iByte++)

      return RESULT_OK;
    } // DecodeRunLengthBinary()
  } // namespace Embedded
} // namespace Anki
