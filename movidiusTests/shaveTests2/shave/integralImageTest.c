#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

void ScrollingIntegralImage_u8_s32_FilterRow_shaveInnerLoop(const int * restrict integralImage_00, const int * restrict integralImage_01, const int * restrict integralImage_10, const int * restrict integralImage_11, const int minXSimd, const int maxXSimd, int * restrict output)
{
  int x;

  const int4 * restrict integralImageX4_00  = &(integralImage_00[0]);

  const int4 * restrict integralImageX4_10  = &(integralImage_10[0]);

  const int4 * restrict integralImageX4_01a = &(integralImage_01[-1]);
  const int4 * restrict integralImageX4_01b = &(integralImage_01[3]);

  const int4 * restrict integralImageX4_11a = &(integralImage_11[-1]);
  const int4 * restrict integralImageX4_11b = &(integralImage_11[3]);

  int4 * restrict outputX4 = &(output[0]);

  __asm(
  ".set integralImage_00_address i20 \n"
    ".set integralImage_10_address i21 \n"
    ".set integralImage_01a_address i22 \n"
    ".set integralImage_01b_address i23 \n"
    ".set integralImage_11a_address i24 \n"
    ".set integralImage_11b_address i25 \n"
    ".set output_address i26 \n"
    "nop 5 \n"
    "cmu.cpii integralImage_00_address %0 \n"
    "cmu.cpii integralImage_10_address %1 \n"
    "cmu.cpii integralImage_01a_address %2 \n"
    "cmu.cpii integralImage_01b_address %3 \n"
    "cmu.cpii integralImage_11a_address %4 \n"
    "cmu.cpii integralImage_11b_address %5 \n"
    "cmu.cpii output_address %6 \n"
    : //Output registers
  :"r"(&integralImageX4_00[0]), "r"(&integralImageX4_10[0]), "r"(&integralImageX4_01a[0]), "r"(&integralImageX4_01b[0]), "r"(&integralImageX4_11a[0]), "r"(&integralImageX4_11b[0]), "r"(&outputX4[0]) //Input registers
    :"i20", "i21", "i22", "i23", "i24", "i25", "i26" //Clobbered registers
    );

  for(x=minXSimd; x<maxXSimd; x+=4) {
    __asm(
    ".set integralImage_00 v0 \n"
      ".set integralImage_10 v1 \n"
      ".set integralImage_01 v2 \n"
      ".set integralImage_01a v3 \n"
      ".set integralImage_01b v4 \n"
      ".set integralImage_11 v5 \n"
      ".set integralImage_11a v6 \n"
      ".set integralImage_11b v7 \n"
      ".set output v8 \n"
      "lsu0.ldxv integralImage_00 integralImage_00_address || lsu1.ldxv integralImage_10 integralImage_10_address\n"
      "nop 1 \n"
      "lsu0.ldxv integralImage_01a integralImage_01a_address || lsu1.ldxv integralImage_01b integralImage_01b_address\n"
      "nop 1 \n"
      "lsu0.ldxv integralImage_11a integralImage_11a_address || lsu1.ldxv integralImage_11b integralImage_11b_address\n"
      "nop 1 \n"
      "iau.add integralImage_00_address integralImage_00_address 16 \n"
      "iau.add integralImage_10_address integralImage_10_address 16 \n"
      "iau.add integralImage_01a_address integralImage_01a_address 16 \n"
      "iau.add integralImage_01b_address integralImage_01b_address 16 \n"
      "iau.add integralImage_11a_address integralImage_11a_address 16 \n"
      "iau.add integralImage_11b_address integralImage_11b_address 16 \n"
      "nop 1 \n"
      "vau.alignvec integralImage_01 integralImage_01a integralImage_01b 4 \n"
      "vau.alignvec integralImage_11 integralImage_11a integralImage_11b 4 \n"
      "nop 1 \n"
      "vau.sub.i32 output integralImage_11 integralImage_10\n" //output[x] = integralImage_11[x] - integralImage_10[x] + integralImage_00[x] - integralImage_01[x];
      "nop 1 \n"
      "vau.add.i32 output output integralImage_00 \n"
      "nop 1 \n"
      "vau.sub.i32 output output integralImage_01 \n"
      "nop 1 \n"
      "lsu0.stxv output output_address \n"
      "iau.add output_address output_address 16 \n \n"
      : //Output registers
    ://Input registers
    :"i20", "i21", "i22", "i23", "i24", "i25", "i26", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8" //Clobbered registers
      );
  }
}

void testIntegralImage() {
  const int filterHalfWidth = 2;

  const int * restrict integralImage_00 = &integralImage[0];
  const int * restrict integralImage_01 = &integralImage[5];
  const int * restrict integralImage_10 = &integralImage[5*IMAGE_HEIGHT + 0];
  const int * restrict integralImage_11 = &integralImage[5*IMAGE_HEIGHT + 5];

  const int minXSimd = 0;
  const int maxXSimd = IMAGE_WIDTH - 2*filterHalfWidth - 1;

  int * restrict output = &outputLine[0];

  ScrollingIntegralImage_u8_s32_FilterRow_shaveInnerLoop(integralImage_00, integralImage_01, integralImage_10, integralImage_11, minXSimd, maxXSimd,output);
}