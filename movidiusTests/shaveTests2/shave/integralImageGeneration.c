#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include <mv_types.h>
#include "shaveShared.h"

#define INNER_LOOP_VERSION 2

void ScrollingIntegralImage_u8_s32_ComputeIntegralImageRow_nthRow(const u8 * restrict paddedImage_currentRow, const s32 * restrict integralImage_previousRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth)
{
#if INNER_LOOP_VERSION == 1
  s32 x;
  s32 horizontalSum = 0;

  for(x=0; x<integralImageWidth; x++) {
    horizontalSum += paddedImage_currentRow[x];
    integralImage_currentRow[x] = horizontalSum + integralImage_previousRow[x];
    //integralImage_currentRow[x] = horizontalSum;
  } // for(x=0; x<integralImageWidth; x++)
#elif INNER_LOOP_VERSION == 2
  s32 x;

  __asm(
  ".set paddedImage_currentRow_address i20 \n"
    ".set integralImage_previousRow_address i21 \n"
    ".set integralImage_currentRow_address i22 \n"
    ".set integralImageWidth i15 \n"
    ".set horizontalSum i23 \n"
    "nop 5 \n"
    "cmu.cpii paddedImage_currentRow_address %0 || iau.sub horizontalSum horizontalSum horizontalSum ; horizontalSum = 0 \n"
    "cmu.cpii integralImage_previousRow_address %1 \n"
    "cmu.cpii integralImage_currentRow_address %2 \n"
    : //Output registers
  :"r"(&paddedImage_currentRow[0]), "r"(&integralImage_previousRow[0]), "r"(&integralImage_currentRow[0]) //Input registers
    :"i20", "i21", "i22", "i23" //Clobbered registers
    );

  for(x=0; x<integralImageWidth; x++) {
    __asm(
    ".set paddedImage_currentRow i24 \n"
      ".set integralImage_previousRow i25 \n"
      ".set integralImage_currentRow i26 \n"
      "lsu0.ldi32.u8.u32 paddedImage_currentRow paddedImage_currentRow_address || lsu1.ldi32 integralImage_previousRow integralImage_previousRow_address \n"
      "nop 5 \n"
      "iau.add horizontalSum horizontalSum paddedImage_currentRow \n"
      "nop 5\n"
      "iau.add integralImage_currentRow horizontalSum integralImage_previousRow \n"
      "nop 5\n"
      "lsu0.sti32 integralImage_currentRow integralImage_currentRow_address \n"
      "nop 5\n"
      : //Output registers
    ://Input registers
    :"i20", "i21", "i22", "i23", "i24", "i25", "i26" //Clobbered registers
      );
  } // for(x=0; x<integralImageWidth; x++)
#endif
}