#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include <mv_types.h>
#include "shaveShared.h"

#define INNER_LOOP_VERSION 5

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
#elif INNER_LOOP_VERSION == 3
  __asm(
  ".set x i10 \n"
    ".set paddedImage_currentRow_address i20 \n"
    ".set integralImage_previousRow_address i21 \n"
    ".set integralImage_currentRow_address i22 \n"
    ".set integralImageWidth i15 \n"
    ".set horizontalSum i23 \n"
    "nop 5 \n"
    "cmu.cpii paddedImage_currentRow_address %0 || iau.sub horizontalSum horizontalSum horizontalSum ; horizontalSum = 0 \n"
    "cmu.cpii integralImage_previousRow_address %1 || iau.sub x x x \n"
    "cmu.cpii integralImage_currentRow_address %2 \n"
    : //Output registers
  :"r"(&paddedImage_currentRow[0]), "r"(&integralImage_previousRow[0]), "r"(&integralImage_currentRow[0]) //Input registers
    :"i10", "i20", "i21", "i22", "i23" //Clobbered registers
    );

  __asm(
  ".set paddedImage_currentRow i24 \n"
    ".set integralImage_previousRow i25 \n"
    ".set integralImage_currentRow i26 \n"
    "; for(x=0; x<integralImageWidth; x++) \n"
    ".lalign \n"
    "_generationLoop: \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow paddedImage_currentRow_address || lsu1.ldi32 integralImage_previousRow integralImage_previousRow_address \n"
    "nop 5 \n"
    "iau.add horizontalSum horizontalSum paddedImage_currentRow \n"
    "iau.add integralImage_currentRow horizontalSum integralImage_previousRow \n"
    "lsu0.sti32 integralImage_currentRow integralImage_currentRow_address \n"
    "iau.add x x 1 \n"
    "cmu.cmii.i32 x integralImageWidth \n"
    "peu.pc1c lt || bru.bra _generationLoop \n"
    "nop 5 \n"
    : //Output registers
  ://Input registers
  :"i10", "i20", "i21", "i22", "i23", "i24", "i25", "i26" //Clobbered registers
    );

#elif INNER_LOOP_VERSION == 4
  const s32 integralImageWidth4 = (integralImageWidth+3) >> 2; // ceil(integralImageWidth/4)
  //const s32 integralImageWidth4 = 2; // ceil(integralImageWidth/4)

  __asm(
  ".set x i10 \n"
    ".set paddedImage_currentRow_address i20 \n"
    ".set integralImage_previousRow_address i21 \n"
    ".set integralImage_currentRow_address i22 \n"
    ".set integralImageWidth i15 \n"
    ".set addressIncrement i0 \n"
    ".set horizontalSum i23 \n"
    "cmu.cpii paddedImage_currentRow_address %0 || iau.sub horizontalSum horizontalSum horizontalSum ; horizontalSum = 0 \n"
    "cmu.cpii integralImage_previousRow_address %1 || iau.sub x x x\n"
    "cmu.cpii integralImage_currentRow_address %2 || lsu1.ldil addressIncrement 16 \n"

    ".set integralImageWidth4 %3 \n"
    ".set paddedImage_currentRow0 i25 \n"
    ".set paddedImage_currentRow1 i26 \n"
    ".set paddedImage_currentRow2 i27 \n"
    ".set paddedImage_currentRow3 i28 \n"
    ".set integralImage_previousRow v0 \n"
    ".set integralImage_currentRow v1 \n"
    ".set integralImage_currentRow.0 v1.0 \n"
    ".set integralImage_currentRow.1 v1.1 \n"
    ".set integralImage_currentRow.2 v1.2 \n"
    ".set integralImage_currentRow.3 v1.3 \n"

    "; for(x=0; x<integralImageWidth4; x++) \n"
    ".lalign \n"
    "_generationLoop: \n"

    "lsu0.ldi32.u8.u32 paddedImage_currentRow0 paddedImage_currentRow_address \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow1 paddedImage_currentRow_address \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow2 paddedImage_currentRow_address \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow3 paddedImage_currentRow_address \n"
    "lsu0.ldxvi integralImage_previousRow integralImage_previousRow_address addressIncrement \n"
    "nop 1 \n"
    "iau.add horizontalSum horizontalSum paddedImage_currentRow0 \n"
    "iau.add horizontalSum horizontalSum paddedImage_currentRow1 || cmu.cpiv integralImage_currentRow.0 horizontalSum \n"
    "iau.add horizontalSum horizontalSum paddedImage_currentRow2 || cmu.cpiv integralImage_currentRow.1 horizontalSum \n"
    "iau.add horizontalSum horizontalSum paddedImage_currentRow3 || cmu.cpiv integralImage_currentRow.2 horizontalSum \n"
    "cmu.cpiv integralImage_currentRow.3 horizontalSum \n"
    "vau.add.i32 integralImage_currentRow integralImage_currentRow integralImage_previousRow \n"
    "nop 1 \n"

    "lsu0.stxvi integralImage_currentRow integralImage_currentRow_address addressIncrement \n"

    "iau.add x x 1 \n"
    "cmu.cmii.i32 x integralImageWidth4 \n"
    "peu.pc1c lt || bru.bra _generationLoop \n"
    "nop 5 \n"
    : //Output registers
  :"r"(&paddedImage_currentRow[0]), "r"(&integralImage_previousRow[0]), "r"(&integralImage_currentRow[0]), "r"(integralImageWidth4)//Input registers
    :"i10", "i20", "i21", "i22", "i23", "i24", "i25", "i26", "v0", "v1" //Clobbered registers
    );

#elif INNER_LOOP_VERSION == 5
  const s32 integralImageWidth4 = (integralImageWidth+3) >> 2; // ceil(integralImageWidth/4)
  //const s32 integralImageWidth4 = 2; // ceil(integralImageWidth/4)

  __asm(
  ".set x i10 \n"
    ".set paddedImage_currentRow_address i20 \n"
    ".set integralImage_previousRow_address i21 \n"
    ".set integralImage_currentRow_address i22 \n"
    ".set integralImageWidth i15 \n"
    ".set addressIncrement i0 \n"
    ".set horizontalSum i23 \n"
    "cmu.cpii paddedImage_currentRow_address %0 || iau.sub horizontalSum horizontalSum horizontalSum ; horizontalSum = 0 \n"
    "cmu.cpii integralImage_previousRow_address %1 || iau.sub x x x \n" //|| lsu0.ldil x 1 || lsu1.ldih x 0 \n"
    "cmu.cpii integralImage_currentRow_address %2 || lsu1.ldil addressIncrement 16 \n"

    ".set integralImageWidth4 %3 \n"
    ".set paddedImage_currentRow0 i25 \n"
    ".set paddedImage_currentRow1 i26 \n"
    ".set paddedImage_currentRow2 i27 \n"
    ".set paddedImage_currentRow3 i28 \n"
    ".set paddedImage_currentRow0_next i5 \n"
    ".set paddedImage_currentRow1_next i6 \n"
    ".set paddedImage_currentRow2_next i7 \n"
    ".set paddedImage_currentRow3_next i8 \n"
    ".set integralImage_previousRow v0 \n"
    ".set integralImage_currentRow v1 \n"
    ".set integralImage_currentRow.0 v1.0 \n"
    ".set integralImage_currentRow.1 v1.1 \n"
    ".set integralImage_currentRow.2 v1.2 \n"
    ".set integralImage_currentRow.3 v1.3 \n"
    ".set integralImage_previousRow_next v2 \n"
    ".set integralImage_currentRow_next v3 \n"

    "lsu0.ldi32.u8.u32 paddedImage_currentRow0_next paddedImage_currentRow_address \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow1_next paddedImage_currentRow_address \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow2_next paddedImage_currentRow_address \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow3_next paddedImage_currentRow_address \n"
    "lsu0.ldxvi integralImage_previousRow_next integralImage_previousRow_address addressIncrement \n"
    "nop 1 \n"

    "; for(x=0; x<integralImageWidth4; x++) \n"
    ".lalign \n"
    "_generationLoop: \n"

    "lsu0.ldi32.u8.u32 paddedImage_currentRow0_next paddedImage_currentRow_address || cmu.cpii paddedImage_currentRow0 paddedImage_currentRow0_next || iau.add x x 1\n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow1_next paddedImage_currentRow_address || cmu.cpii paddedImage_currentRow1 paddedImage_currentRow1_next \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow2_next paddedImage_currentRow_address || cmu.cpii paddedImage_currentRow2 paddedImage_currentRow2_next \n"
    "lsu0.ldi32.u8.u32 paddedImage_currentRow3_next paddedImage_currentRow_address || cmu.cpii paddedImage_currentRow3 paddedImage_currentRow3_next \n"
    "lsu0.ldxvi integralImage_previousRow_next integralImage_previousRow_address addressIncrement || cmu.cpvv integralImage_previousRow integralImage_previousRow_next || iau.add horizontalSum horizontalSum paddedImage_currentRow0 \n "
    "iau.add horizontalSum horizontalSum paddedImage_currentRow1 || cmu.cpiv integralImage_currentRow.0 horizontalSum \n"
    "iau.add horizontalSum horizontalSum paddedImage_currentRow2 || cmu.cpiv integralImage_currentRow.1 horizontalSum \n"
    "iau.add horizontalSum horizontalSum paddedImage_currentRow3 || cmu.cpiv integralImage_currentRow.2 horizontalSum \n"
    "cmu.cmii.i32 x integralImageWidth4 ; This is checking x for the next iteration \n"
    "peu.pc1c lt || bru.bra _generationLoop \n"
    "cmu.cpiv integralImage_currentRow.3 horizontalSum \n"
    "vau.add.i32 integralImage_currentRow integralImage_currentRow integralImage_previousRow\n"
    "nop 1 \n"
    "lsu0.stxvi integralImage_currentRow integralImage_currentRow_address addressIncrement \n"
    "nop 1 \n"
    "nop 4 ; these nops are only executed at the end of the final iteration \n"
    : //Output registers
  :"r"(&paddedImage_currentRow[0]), "r"(&integralImage_previousRow[0]), "r"(&integralImage_currentRow[0]), "r"(integralImageWidth4)//Input registers
    :"i10", "i20", "i21", "i22", "i23", "i24", "i25", "i26", "v0", "v1" //Clobbered registers
    );

#endif
}