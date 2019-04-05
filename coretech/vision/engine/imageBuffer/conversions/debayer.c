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

#include <time.h>
#include <stdio.h>
#include <errno.h>

#define CLIP(in__, out__) out__ = ( ((in__) < 0) ? 0 : ((in__) > 255 ? 255 : (in__)) )

const uint8_t LUT[128] = { 0,  28,  39,  46,  53,  59,  64,  68,
                           73,  77,  80,  84,  87,  90,  94,  97,
                           99, 102, 105, 108, 110, 113, 115, 117,
                           120, 122, 124, 126, 128, 130, 132, 134,
                           136, 138, 140, 142, 144, 146, 147, 149,
                           151, 153, 154, 156, 158, 159, 161, 162,
                           164, 165, 167, 168, 170, 171, 173, 174,
                           176, 177, 179, 180, 181, 183, 184, 185,
                           187, 188, 189, 191, 192, 193, 195, 196,
                           197, 198, 199, 201, 202, 203, 204, 206,
                           207, 208, 209, 210, 211, 212, 214, 215,
                           216, 217, 218, 219, 220, 221, 222, 223,
                           225, 226, 227, 228, 229, 230, 231, 232,
                           233, 234, 235, 236, 237, 238, 239, 240,
                           241, 242, 243, 244, 245, 246, 247, 248,
                           249, 249, 250, 251, 252, 253, 254, 255};

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

  /* static int b = 0; */
  /* if(b == 0) */
  /*   { */
  /* FILE* f = fopen("/tmp/test.raw", "w"); */
  /* if(f != NULL) */
  /* { */
  /*   fwrite(bayer_in, 1600*720, 1, f); */
  /*   fclose(f); */
  /* } */
  /* b = 1; */
  /*   } */
  /* else */
  /*   { */
  /*     printf("FAILED %d\n", errno); */
  /*   } */

  clock_t start = clock();

  // input width must be divisble by bpp
  assert((bayer_sx % bpp) == 0);

  // Each iteration of the inline assembly processes 20 bytes at a time
  const uint8_t  kNumBytesProcessedPerLoop = 20;
  assert((bayer_sx % kNumBytesProcessedPerLoop) == 0);

  const uint32_t kNumInnerLoops = bayer_sx / kNumBytesProcessedPerLoop;
  const uint32_t kNumOuterLoops = bayer_sy >> 1;

  uint8_t* bayer = (uint8_t*)bayer_in;
  uint8_t* bayer2 = bayer + bayer_sx;
  uint8_t* lut = (uint8_t*)LUT;

  int i = 0;
  int j = 1;

  #define GAMMA 1

  __asm__ volatile
  (
#if GAMMA
   "VLD1.8 {d8}, [%[lut]]! \n\t"
   "VLD1.8 {d9}, [%[lut]]! \n\t"
   "VLD1.8 {d10}, [%[lut]]! \n\t"
   "VLD1.8 {d11}, [%[lut]]! \n\t"
   "VLD1.8 {d12}, [%[lut]]! \n\t"
   "VLD1.8 {d13}, [%[lut]]! \n\t"
   "VLD1.8 {d14}, [%[lut]]! \n\t"
   "VLD1.8 {d15}, [%[lut]]! \n\t"
   "VLD1.8 {d16}, [%[lut]]! \n\t"
   "VLD1.8 {d17}, [%[lut]]! \n\t"
   "VLD1.8 {d18}, [%[lut]]! \n\t"
   "VLD1.8 {d19}, [%[lut]]! \n\t"
   "VLD1.8 {d20}, [%[lut]]! \n\t"
   "VLD1.8 {d21}, [%[lut]]! \n\t"
   "VLD1.8 {d22}, [%[lut]]! \n\t"
   "VLD1.8 {d23}, [%[lut]]! \n\t"

   "VMOV.I8 d24, #0x20 \n\t"
#endif

   "1:\n\t"
   "CMP %[i], %[numOuter]\n\t"
   "BEQ 4f \n\t"

   "MOV %[j], 0 \n\t"

   "2:\n\t"

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

#if !GAMMA
   // Perform a saturating left shift on all elements since each color byte is the 8 high bits
   // from the 10 bit data. This is like adding 00 as the two low bits
   "VQSHL.U8 d0, d0, #2 \n\t"
   "VQSHL.U8 d1, d1, #2 \n\t"
   "VQSHL.U8 d2, d2, #2 \n\t"
#else
   "VMOV.I8 d5, #0 \n\t"
   "VMOV.I8 d6, #0 \n\t"
   "VMOV.I8 d7, #0 \n\t"

   "VSRI.8 d5, d0, #1 \n\t"
   "VSRI.8 d6, d1, #1 \n\t"
   "VSRI.8 d7, d2, #1 \n\t"

   "VTBX.8 d0, {d8, d9, d10, d11}, d5 \n\t"
   "VTBX.8 d1, {d8, d9, d10, d11}, d6 \n\t"
   "VTBX.8 d2, {d8, d9, d10, d11}, d7 \n\t"
   "VSUB.I8 d5, d5, d24 \n\t"
   "VSUB.I8 d6, d6, d24 \n\t"
   "VSUB.I8 d7, d7, d24 \n\t"

   "VTBX.8 d0, {d12, d13, d14, d15}, d5 \n\t"
   "VTBX.8 d1, {d12, d13, d14, d15}, d6 \n\t"
   "VTBX.8 d2, {d12, d13, d14, d15}, d7 \n\t"
   "VSUB.I8 d5, d5, d24 \n\t"
   "VSUB.I8 d6, d6, d24 \n\t"
   "VSUB.I8 d7, d7, d24 \n\t"

   "VTBX.8 d0, {d16, d17, d18, d19}, d5 \n\t"
   "VTBX.8 d1, {d16, d17, d18, d19}, d6 \n\t"
   "VTBX.8 d2, {d16, d17, d18, d19}, d7 \n\t"
   "VSUB.I8 d5, d5, d24 \n\t"
   "VSUB.I8 d6, d6, d24 \n\t"
   "VSUB.I8 d7, d7, d24 \n\t"

   "VTBX.8 d0, {d20, d21, d22, d23}, d5 \n\t"
   "VTBX.8 d1, {d20, d21, d22, d23}, d6 \n\t"
   "VTBX.8 d2, {d20, d21, d22, d23}, d7 \n\t"
#endif

   // Interleaving store of red, green, and blue bytes into output rgb image
   "VST3.8 {d0, d1, d2}, [%[out]]! \n\t"

   "CMP %[j], %[numInner] \n\t"
   "BEQ 3f \n\t"
   "ADD %[j], %[j], #1 \n\t"
   "B 2b \n\t"

   "3:\n\t"
   "ADD %[i], %[i], #1 \n\t"
   // Skip a row
   "ADD %[ptr], %[ptr], %[bayer_sx]\n\t"
   "ADD %[ptr2], %[ptr2], %[bayer_sx]\n\t"
   "B 1b \n\t"

   "4:\n\t"

   :
   : [ptr] "r" (bayer),
     [out] "r" (rgb),
     [ptr2] "r" (bayer2),
     [numOuter] "r" (kNumOuterLoops-1),
     [numInner] "r" (kNumInnerLoops-1),
     [bayer_sx] "r" (bayer_sx),
     [lut] "r" (lut),
     [i] "r" (i),
     [j] "r" (j)

   : "d0", "d1", "d2", "d3", "d4", "d5",
     "d6","d7","d8","d9","d10","d11",
     "d12","d13","d14","d15","d16",
     "d17","d18","d19","d20","d21",
     "d22","d23", "d24", "memory" // Clobber list of registers used and memory since it is being written to
      );

  clock_t end = clock();
  static double avg = 0;
  static int count = 0;
  avg += ((double)(end-start)/CLOCKS_PER_SEC);
  count++;

  printf("wow %f\n", avg/(double)count);
}

#else

void bayer_mipi_bggr10_downsample(const uint8_t *bayer, uint8_t *rgb, int bayer_sx, int bayer_sy, int bpp)
{
  // input width must be divisble by bpp
  assert((bayer_sx % bpp) == 0);

  uint8_t *outR, *outG, *outB;
  register int i, j;
  int tmp;

  outR = &rgb[0];
  outG = &rgb[1];
  outB = &rgb[2];

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

int main()
{
  for(int i = 0; i < 100; ++i)
    {
      FILE* f = fopen("/test.raw", "r+");
      if(f != NULL)
        {
          uint8_t buf[1600*720];
          (void)fread(&buf, sizeof(buf), 1, f);

          uint8_t out[640*360*3];
          bayer_mipi_bggr10_downsample((uint8_t*)&buf, (uint8_t*)&out, 1600, 720, 10);
        }
      fclose(f);
    }
}
