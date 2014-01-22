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

        AnkiConditionalErrorAndReturnValue(res == HSER_SINK_OK,
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

          AnkiConditionalErrorAndReturnValue(res == HSDR_FINISH_DONE,
            RESULT_FAIL, "Decompress", "heatshrink_decoder_finish failed");
        }
      } // while (sunk < inLength)

      outUncompressedLength = polled;

      return RESULT_OK;
    } // Result Decompress()
  } // namespace Embedded
} // namespace Anki