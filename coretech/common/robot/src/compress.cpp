/**
File: compress.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/compress.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/errorHandling.h"

#ifdef ANKICORETECH_EMBEDDED_USE_HEATSHRINK
#include "heatshrink_encoder.h"
#include "heatshrink_decoder.h"

#if HEATSHRINK_DYNAMIC_ALLOC
#error HEATSHRINK_DYNAMIC_ALLOC must be false for static allocation test suite.
#endif
#endif // #ifdef ANKICORETECH_EMBEDDED_USE_HEATSHRINK

namespace Anki
{
  namespace Embedded
  {
#ifdef ANKICORETECH_EMBEDDED_USE_HEATSHRINK
    Result Compress(const void * in, const u32 inLength, void *out, const u32 outMaxLength, s32 &outCompressedLength, MemoryStack scratch)
    {
      heatshrink_encoder *hse = reinterpret_cast<heatshrink_encoder*>( scratch.Allocate(sizeof(heatshrink_encoder)) );

      outCompressedLength = -1;

      heatshrink_encoder_reset(hse);

      size_t count = 0;

      uint32_t sunk = 0;
      uint32_t polled = 0;
      while (sunk < inLength) {
        const HSE_sink_res res = heatshrink_encoder_sink(hse, const_cast<u8*>(&reinterpret_cast<const u8*>(in)[sunk]), inLength - sunk, &count);

        AnkiConditionalErrorAndReturnValue(res == HSER_SINK_OK,
          RESULT_FAIL, "Compress", "heatshrink_encoder_sink failed");

        sunk += count;

        if (sunk == inLength) {
          const HSE_finish_res res = heatshrink_encoder_finish(hse);

          AnkiConditionalErrorAndReturnValue(res == HSER_FINISH_MORE,
            RESULT_FAIL, "Compress", "heatshrink_encoder_finish failed");
        }

        HSE_poll_res pres;
        //if(polled < inLength) {
        do { // "turn the crank"
          pres = heatshrink_encoder_poll(hse, &reinterpret_cast<u8*>(out)[polled], outMaxLength - polled, &count);
          polled += count;
        } while (pres == HSER_POLL_MORE);
        //}

        AnkiConditionalErrorAndReturnValue(pres == HSER_POLL_EMPTY,
          RESULT_FAIL, "Compress", "pres is not empty");

        AnkiConditionalErrorAndReturnValue(polled < outMaxLength,
          RESULT_FAIL, "Compress", "compression should never expand that much");

        if (sunk == inLength) {
          const HSE_finish_res res = heatshrink_encoder_finish(hse);

          AnkiConditionalErrorAndReturnValue(res == HSER_FINISH_DONE,
            RESULT_FAIL, "Compress", "heatshrink_encoder_finish failed");
        }
      } // while (sunk < inLength)

      outCompressedLength = polled;

      return RESULT_OK;
    } // Result Compress()

    Result Decompress(const void * in, const u32 inLength, void *out, const u32 outMaxLength, s32 &outUncompressedLength, MemoryStack scratch)
    {
      heatshrink_decoder *hsd = reinterpret_cast<heatshrink_decoder*>( scratch.Allocate(sizeof(heatshrink_decoder)) );

      outUncompressedLength = -1;

      heatshrink_decoder_reset(hsd);

      u32 sunk = 0;
      u32 polled = 0;
      size_t count = 0;

      while (sunk < inLength) {
        const HSD_sink_res res = heatshrink_decoder_sink(hsd, const_cast<u8*>(&reinterpret_cast<const u8*>(in)[sunk]), inLength - sunk, &count);

        AnkiConditionalErrorAndReturnValue(res == HSDR_SINK_OK,
          RESULT_FAIL, "Decompress", "heatshrink_decoder_sink failed");

        sunk += count;

        if (sunk == inLength) {
          const HSD_finish_res res = heatshrink_decoder_finish(hsd);

          AnkiConditionalErrorAndReturnValue(res == HSDR_FINISH_MORE,
            RESULT_FAIL, "Decompress", "heatshrink_decoder_finish failed");
        }

        HSD_poll_res pres;
        do {
          pres = heatshrink_decoder_poll(hsd, &reinterpret_cast<u8*>(out)[polled], outMaxLength - polled, &count);
          polled += count;
        } while (pres == HSDR_POLL_MORE);

        AnkiConditionalErrorAndReturnValue(pres == HSDR_POLL_EMPTY,
          RESULT_FAIL, "Decompress", "pres is not empty");

        if (sunk == inLength) {
          const HSD_finish_res fres = heatshrink_decoder_finish(hsd);

          AnkiConditionalErrorAndReturnValue(fres == HSDR_FINISH_DONE,
            RESULT_FAIL, "Decompress", "heatshrink_decoder_finish failed");
        }
      } // while (sunk < inLength)

      outUncompressedLength = polled;

      return RESULT_OK;
    } // Result Decompress()
#endif // #ifdef ANKICORETECH_EMBEDDED_USE_HEATSHRINK

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
