#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveShared.h"

#define INNER_LOOP_VERSION 2

void ScrollingIntegralImage_u8_s32_FilterRow_shaveInnerLoop(const int * restrict integralImage_00, const int * restrict integralImage_01, const int * restrict integralImage_10, const int * restrict integralImage_11, const int numPixelsToProcess, int * restrict output)
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

#if INNER_LOOP_VERSION == 1
  for(x=0; x<numPixelsToProcess; x+=4) {
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
#elif INNER_LOOP_VERSION == 2
  //for(x=minXSimd; x<maxXSimd; x+=4) {
  // Warning: doesn't check if minXSimd > maxXSimd
  __asm(
  ".set x i10 \n"
    ".set numPixelsToProcess i14 \n"
    ".set addressIncrement i0 \n"
    ".set integralImage_00 v0 \n"
    ".set integralImage_00_next v1 \n"
    ".set integralImage_10 v2 \n"
    ".set integralImage_10_next v3 \n"
    ".set integralImage_01 v4 \n"
    ".set integralImage_01_next v5 \n"
    ".set integralImage_01a v6 \n"
    ".set integralImage_01a_next v7 \n"
    ".set integralImage_01b v8 \n"
    ".set integralImage_01b_next v9 \n"
    ".set integralImage_11 v10 \n"
    ".set integralImage_11_next v11 \n"
    ".set integralImage_11a v12 \n"
    ".set integralImage_11a_next v13 \n"
    ".set integralImage_11b v14 \n"
    ".set integralImage_11b_next v15 \n"
    ".set output v16 \n"
    ".set outputTmp v17 \n"
    "\n"
    "; for(x=0; x<numPixelsToProcess; x+=4) \n"
    "lsu0.ldil x 0 || lsu1.ldil addressIncrement 16 \n"
    "; preload the first set of input pixels \n"
    "lsu0.ldxvi integralImage_00 integralImage_00_address addressIncrement || lsu1.ldxvi integralImage_10 integralImage_10_address addressIncrement\n"
    "nop 1 \n"
    "lsu0.ldxvi integralImage_01a integralImage_01a_address addressIncrement || lsu1.ldxvi integralImage_01b integralImage_01b_address addressIncrement\n"
    "nop 1 \n"
    "lsu0.ldxvi integralImage_11a integralImage_11a_address addressIncrement || lsu1.ldxvi integralImage_11b integralImage_11b_address addressIncrement\n"
    "nop 6 \n"
    "vau.alignvec integralImage_01 integralImage_01a integralImage_01b 4 \n"
    "vau.alignvec integralImage_11 integralImage_11a integralImage_11b 4 \n"
    ".lalign \n"
    "_filteringLoop: \n"
    "lsu0.ldxvi integralImage_01a_next integralImage_01a_address addressIncrement || lsu1.ldxvi integralImage_01b_next integralImage_01b_address addressIncrement\n"
    "nop 1 \n"
    "lsu0.ldxvi integralImage_11a_next integralImage_11a_address addressIncrement || lsu1.ldxvi integralImage_11b_next integralImage_11b_address addressIncrement\n"
    "nop 1 \n"
    "lsu0.ldxvi integralImage_00_next integralImage_00_address addressIncrement || lsu1.ldxvi integralImage_10_next integralImage_10_address addressIncrement\n"
    "nop 1 \n"
    "vau.sub.i32 outputTmp integralImage_11 integralImage_10 || iau.add x x 4 \n"
    "vau.sub.i32 output integralImage_00 integralImage_01\n"
    "nop 1 \n"
    "vau.add.i32 output output outputTmp \n"
    "nop 1 \n"
    "lsu0.stxvi output output_address addressIncrement || vau.alignvec integralImage_01_next integralImage_01a_next integralImage_01b_next 4\n"
    "vau.alignvec integralImage_11_next integralImage_11a_next integralImage_11b_next 4 \n"
    "cmu.cpvv integralImage_01 integralImage_01_next \n"
    "cmu.cpvv integralImage_11 integralImage_11_next \n"
    "cmu.cpvv integralImage_00 integralImage_00_next \n"
    "cmu.cpvv integralImage_10 integralImage_10_next \n"
    "cmu.cmii.i32 x numPixelsToProcess \n"
    "peu.pc1c lt || bru.bra _filteringLoop \n"
    "nop 5 \n"

    //"; for(x=0; x<numPixelsToProcess; x+=4) \n"
    //"lsu0.ldil x 0 || lsu1.ldil addressIncrement 16 \n"
    //".lalign \n"
    //"_filteringLoop: \n"
    //"lsu0.ldxvi integralImage_01a_next integralImage_01a_address addressIncrement \n"
    //"lsu1.ldxvi integralImage_01b_next integralImage_01b_address addressIncrement \n"
    //"nop 1 \n"
    //"nop 5 \n"
    //"nop 5 \n"
    //"cmu.cpvv integralImage_01a integralImage_01a_next \n"
    //"nop 5 \n"
    //"cmu.cpvv integralImage_01b integralImage_01b_next \n"
    //"nop 5 \n"
    //"lsu0.ldxvi integralImage_11a_next integralImage_11a_address addressIncrement \n"
    //"lsu1.ldxvi integralImage_11b_next integralImage_11b_address addressIncrement \n"
    //"nop 1 \n"
    //"nop 5 \n"
    //"nop 5 \n"
    //"cmu.cpvv integralImage_11a integralImage_11a_next \n"
    //"nop 5 \n"
    //"cmu.cpvv integralImage_01b integralImage_11b_next \n"
    //"nop 5 \n"
    //"lsu0.ldxvi integralImage_00_next integralImage_00_address addressIncrement \n"
    //"lsu1.ldxvi integralImage_10_next integralImage_10_address addressIncrement \n"
    //"nop 1 \n"
    //"nop 5 \n"
    //"nop 5 \n"
    //"cmu.cpvv integralImage_00 integralImage_00_next \n"
    //"nop 5 \n"
    //"cmu.cpvv integralImage_10 integralImage_10_next \n"
    //"nop 5 \n"
    //"vau.alignvec integralImage_01 integralImage_01a integralImage_01b 4 \n"
    //"nop 5 \n"
    //"vau.alignvec integralImage_11 integralImage_11a integralImage_11b 4 \n"
    //"nop 5 \n"
    //"lsu0.stxvi integralImage_11b output_address addressIncrement \n"
    //"nop 5 \n"
    //"iau.add x x 4 \n"
    //"nop 1 \n"
    //"nop 5 \n"
    //"cmu.cmii.i32 x numPixelsToProcess \n"
    //"peu.pc1c lt || bru.bra _filteringLoop \n"
    //"nop 5 \n"

    : //Output registers
  ://Input registers
  :"i0", "i10", "i20", "i21", "i22", "i23", "i24", "i25", "i26", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8" //Clobbered registers
    );
  //}
#endif // INNER_LOOP_VERSION
}