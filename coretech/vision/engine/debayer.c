/**
 * File: debayer.c
 *
 * Author: chapados + al
 * Created: 1/29/2018
 *
 * Description: Downsample de-Bayer algorithm for MIPI BGGR 10bit raw images                        
 *              Copied from mm-anki-camera in OS
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "debayer.h"

#include <assert.h>
#include <stdint.h>

#define CLIP(in__, out__) out__ = ( ((in__) < 0) ? 0 : ((in__) > 255 ? 255 : (in__)) )

//
// raw RDI pixel format is CAM_FORMAT_BAYER_MIPI_RAW_10BPP_BGGR
// This appears to be the same as:
// https://www.linuxtv.org/downloads/v4l-dvb-apis-old/pixfmt-srggb10p.html
//
// 4 pixels stored in 5 bytes. Each of the first 4 bytes contain the 8 high order bits
// of the pixel. The 5th byte contains the two least significant bits of each pixel in the
// same order.
//
#ifdef __ARM_NEON__
void bayer_mipi_bggr10_downsample(const uint8_t *bayer_in, uint8_t *rgb, int bayer_sx, int bayer_sy, int bpp)
{
  // input width must be divisble by bpp
  assert((bayer_sx % bpp) == 0);

  // Each iteration of the inline assembly processes 20 bytes at a time
  const uint8_t  kNumBytesProcessedPerLoop = 20;
  assert((bayer_sx % kNumBytesProcessedPerLoop) == 0);

  const uint32_t kNumInnerLoops = bayer_sx / kNumBytesProcessedPerLoop;
  const uint32_t kNumOuterLoops = bayer_sy >> 1;

  uint8_t* bayer = (uint8_t*)bayer_in;
  uint8_t* bayer2 = bayer + bayer_sx;

  uint32_t i, j;
  for (i = 0; i < kNumOuterLoops; ++i) {
    for (j = 0; j < kNumInnerLoops; ++j) {
      __asm__ volatile
      (
        "VLD1.8 {d0}, [%[ptr]] \n\t"  // Load 8 bytes from raw bayer image
        "ADD %[ptr], %[ptr], #5 \n\t" // Increment bayer pointer by 5 (5 byte packing)
        "VLD1.8 {d1}, [%[ptr]] \n\t"  // Load 8 more bytes
        "VSHL.I64 d0, d0, #32 \n\t"   // Shift out the partial packed bytes from d0
        "ADD %[ptr], %[ptr], #5 \n\t" // Increment bayer pointer again, after ld and shl so they can be dual issued
        "VEXT.8 d0, d0, d1, #4 \n\t"  // Extract first 4 bytes from d0 and following 4 bytes from d1
        // d0 is now alternating red and green bytes

        // Repeat above steps for next 8 bytes
        "VLD1.8 {d1}, [%[ptr]] \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t"
        "VLD1.8 {d2}, [%[ptr]] \n\t"
        "VSHL.I64 d1, d1, #32 \n\t"
        "ADD %[ptr], %[ptr], #5 \n\t"
        "VEXT.8 d1, d1, d2, #4 \n\t" // d1 is alternating red and green

        "VUZP.8 d0, d1 \n\t" // Unzip alternating bytes so d0 is all red bytes and d1 is all green bytes

        // Repeat above for the second row containing green and blue bytes
        "VLD1.8 {d2}, [%[ptr2]] \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VLD1.8 {d3}, [%[ptr2]] \n\t"
        "VSHL.I64 d2, d2, #32 \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VEXT.8 d2, d2, d3, #4 \n\t" // d2 is alternating green and blue

        "VLD1.8 {d3}, [%[ptr2]] \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VLD1.8 {d4}, [%[ptr2]] \n\t"
        "VSHL.I64 d3, d3, #32 \n\t"
        "ADD %[ptr2], %[ptr2], #5 \n\t"
        "VEXT.8 d3, d3, d4, #4 \n\t" // d3 is alternating green and blue

        "VUZP.8 d2, d3 \n\t" // d2 is green, d3 is blue
        "VSWP.8 d2, d3 \n\t" // Due to required register stride on saving need to have blue in d2 so swap

        // Average the green data with a vector halving add
        // Could save an instruction by just ignoring the second set of green data (d3)
        "VHADD.U8 d1, d1, d3 \n\t"

        // Perform a saturating left shift on all elements since each color byte is the 8 high bits
        // from the 10 bit data. This is like adding 00 as the two low bits
        "VQSHL.U8 d0, d0, #2 \n\t"
        "VQSHL.U8 d1, d1, #2 \n\t"
        "VQSHL.U8 d2, d2, #2 \n\t"

        // Interleaving store of red, green, and blue bytes into output rgb image
        "VST3.8 {d0, d1, d2}, [%[out]]! \n\t"

        : [ptr] "+r"(bayer),   // Output list because we want the output of the pointer adds, + since we are reading
          [out] "+r"(rgb),     //   and writing from these registers
          [ptr2] "+r"(bayer2)
        : // empty input list
        : "d0", "d1", "d2", "d3", "d4", "memory" // Clobber list of registers used and memory since it is being written to
      );
    }

    // Skip a row
    bayer += bayer_sx;
    bayer2 += bayer_sx;
  }
}

#else

void bayer_mipi_bggr10_downsample(const uint8_t *bayer, uint8_t *rgb, int bayer_sx, int bayer_sy, int bpp)
{
  // input width must be divisble by bpp
  assert((bayer_sx % bpp) == 0);

  uint8_t *outR, *outG, *outB;
  register int i, j;
  int tmp;

  outB = &rgb[0];
  outG = &rgb[1];
  outR = &rgb[2];

  // Raw image are reported as 1280x720, 10bpp BGGR MIPI Bayer format
  // Based on frame metadata, the raw image dimensions are actually 1600x720 10bpp pixels.
  // Simple conversion + downsample to RGB yields: 640x360 images
  const int dim = (bayer_sx * bayer_sy);

  // output rows have 8-bit pixels
  const int out_sx = bayer_sx * 8 / bpp;

  const int dim_step = (bayer_sx << 1);
  const int out_dim_step = (out_sx << 1);

  int out_i, out_j;

  for (i = 0, out_i = 0; i < dim; i += dim_step, out_i += out_dim_step) {
    // process 2 rows at a time
    for (j = 0, out_j = 0; j < bayer_sx; j += 5, out_j += 4) {
      // process 4 col at a time
      // A B A_ B_ -> B G B G
      // C D C_ D_    G R G R

      // Read 4 10-bit px from row0
      const int r0_idx = i + j;
      uint16_t px_A  = (bayer[r0_idx + 0] << 2) | ((bayer[r0_idx + 4] & 0xc0) >> 6);
      uint16_t px_B  = (bayer[r0_idx + 1] << 2) | ((bayer[r0_idx + 4] & 0x30) >> 4);
      uint16_t px_A_ = (bayer[r0_idx + 2] << 2) | ((bayer[r0_idx + 4] & 0x0c) >> 2);
      uint16_t px_B_ = (bayer[r0_idx + 3] << 2) | (bayer[r0_idx + 4] & 0x03);

      // Read 4 10-bit px from row1
      const int r1_idx = (i + bayer_sx) + j;
      uint16_t px_C  = (bayer[r1_idx + 0] << 2) | ((bayer[r1_idx + 4] & 0xc0) >> 6);
      uint16_t px_D  = (bayer[r1_idx + 1] << 2) | ((bayer[r1_idx + 4] & 0x30) >> 4);
      uint16_t px_C_ = (bayer[r1_idx + 2] << 2) | ((bayer[r1_idx + 4] & 0x0c) >> 2);
      uint16_t px_D_ = (bayer[r1_idx + 3] << 2) | (bayer[r1_idx + 4] & 0x03);

      // output index:
      // out_i represents 2 rows -> divide by 4
      // out_j represents 1 col  -> divide by 2
      const int rgb_0_idx = ((out_i >> 2) + (out_j >> 1)) * 3;
      tmp = (px_C + px_B) >> 1;
      CLIP(tmp, outG[rgb_0_idx]);
      tmp = px_D;
      CLIP(tmp, outR[rgb_0_idx]);
      tmp = px_A;
      CLIP(tmp, outB[rgb_0_idx]);

      const int rgb_1_idx = ((out_i >> 2) + ((out_j + 2) >> 1)) * 3;
      tmp = (px_C_ + px_B_) >> 1;
      CLIP(tmp, outG[rgb_1_idx]);
      tmp = px_D_;
      CLIP(tmp, outR[rgb_1_idx]);
      tmp = px_A_;
      CLIP(tmp, outB[rgb_1_idx]);
    }
  }
}

#endif
