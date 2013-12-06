#include <stdio.h>
#include <math.h>
#include <moviVectorUtils.h>
#include "shaveKernels.h"

#define INNER_LOOP_VERSION 2

__asm(
".data .rodata.vOnes_f32x4 \n"
  ".salign 16 \n"
  "    vOnes_f32x4: \n"
  "        .float32  1.0F32, 1.0F32, 1.0F32, 1.0F32 \n");

#define InterpolateBilinear2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse, interpolatedPixel)\
{\
  const f32 interpolatedTop = alphaXinverse*pixelTL + alphaX*pixelTR;\
  const f32 interpolatedBottom = alphaXinverse*pixelBL + alphaX*pixelBR;\
  interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;\
}

void interp2_shaveInnerLoop(
  const f32 * restrict pXCoordinates, const f32 * restrict pYCoordinates,
  const s32 numCoordinates,
  const u8 * restrict pReference, const int referenceStride,
  const f32 xReferenceMax, const f32 yReferenceMax,
  f32 * restrict pOut)
{
  const f32 invalidValue = -2.0f;
  //const f32 one = 1.0f;

#if INNER_LOOP_VERSION == 1
  s32 x;
  for(x=0; x<numCoordinates; x++) {
    const f32 curX = pXCoordinates[x];
    const f32 curY = pYCoordinates[x];

    const f32 x0 = floorf(curX);
    const f32 x1 = x0 + 1.0f; // ceilf(curX)

    const f32 y0 = floorf(curY);
    const f32 y1 = y0 + 1.0f; // ceilf(curY)

    // If out of bounds, set as invalid and continue
    if(x0 < 0.0f || x1 > xReferenceMax || y0 < 0.0f || y1 > yReferenceMax) {
      pOut[x] = invalidValue;
      continue;
    }

    {
      const f32 alphaX = curX - x0;
      const f32 alphaXinverse = 1 - alphaX;

      const f32 alphaY = curY - y0;
      const f32 alphaYinverse = 1.0f - alphaY;

      const s32 y0S32 = (s32)(floorf(y0));
      const s32 y1S32 = (s32)(floorf(y1));
      const s32 x0S32 = (s32)(floorf(x0));

      const u8 * restrict pReference_y0 = &pReference[y0S32*referenceStride + x0S32];
      const u8 * restrict pReference_y1 = &pReference[y1S32*referenceStride + x0S32];

      const f32 pixelTL = *pReference_y0;
      const f32 pixelTR = *(pReference_y0+1);
      const f32 pixelBL = *pReference_y1;
      const f32 pixelBR = *(pReference_y1+1);

      f32 interpolatedPixel;
      InterpolateBilinear2d(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse, interpolatedPixel);

      pOut[x] = interpolatedPixel;
    }
  } // for(s32 x=0; x<xIterationMax; x++)

#elif INNER_LOOP_VERSION == 2
  //__asm(
  //".set xCoordinates_address i0 \n"
  //  ".set yCoordinates_address i1 \n"
  //  ".set pOut_address i2 \n"
  //  ".set zero i3 \n"
  //  ".set xReferenceMax i4 \n"
  //  ".set yReferenceMax i5 \n"
  //  "nop 5 \n"
  //  "cmu.cpii xCoordinates_address %0 \n"
  //  "cmu.cpii yCoordinates_address %1 \n"
  //  "cmu.cpii pOut_address %2 \n"
  //  "cmu.cpii xReferenceMax %3 \n"
  //  "cmu.cpii yReferenceMax %4 \n"
  //  "iau.sub zero zero zero \n"
  //  : //Output registers
  //:"r"(&pXCoordinates[0]), "r"(&pYCoordinates[0]), "r"(&pOut[0]), "r"(&xReferenceMax), "r"(&yReferenceMax) //Input registers
  //  :"i0", "i1", "i2", "i3", "i4" //Clobbered registers
  //  );

  /*s32 x;
  for(x=0; x<numCoordinates; x++) {
  const f32 curX = pXCoordinates[x];
  const f32 curY = pYCoordinates[x];*/

  __asm(
  ".set iTmp0 i0 ; These four tmp ints may be called different things in [] blocks \n"
    ".set iTmp1 i1 \n"
    ".set iTmp2 i2 \n"
    ".set iTmp3 i3 \n"
    "\n"
    ".set addressIncrement i4 \n"
    "\n"
    ".set invalidValue %0 \n"
    ".set numCoordinates %1 \n"
    ".set xCoordinates_address %2 \n"
    ".set yCoordinates_address %3 \n"
    ".set pOut_address %4 \n"
    ".set xReferenceMax %5 \n"
    ".set yReferenceMax %6 \n"
    "\n"
    ".set p_cfg_save i4 ; used to hold the original state of the p_cfg register, as this function will modify it \n"
    ".set x i5 ; iterator"
    "\n"
    ".set sTmp0 s0 \n"
    ".set sTmp1 s1 \n"
    ".set sTmp2 s2 \n"
    ".set sTmp3 s3 \n"
    "\n"
    ".set vTmp0 v0 ; These four tmp vectors may be called different things in [] blocks \n"
    ".set vTmp1 v1 \n"
    ".set vTmp2 v2 \n"
    ".set vTmp3 v3 \n"
    "\n"
    ".set fillColor_f32x4 v4 \n"
    ".set xCoordinates_f32x4 v5 \n"
    ".set yCoordinates_f32x4 v6 \n"
    ".set x0_i32x4 v7 ; (x0,y0) is the upper-left pixel, (x1,y1) is the lower-right pixel \n"
    ".set y0_i32x4 v8 \n"
    ".set x1_i32x4 v9 \n"
    ".set y1_i32x4 v10 \n"
    ".set ones_f32x4 v11 \n"
    ".set xReferenceMax_i32x4 v12 \n"
    ".set yReferenceMax_i32x4 v13 \n"
    ".set out_f32x4 v14 \n"
    ".set alphaX_f32x4 v15 \n"
    ".set alphaY_f32x4 v16 \n"
    " \n"
    " \n"
    "\n"
    "; save the old rounding mode, then set F2INTRND to round to negative infinty (Also unsets the other flags) \n"
    ";[ \n"
    "  .set cfgMask iTmp0 \n"
    "  .set cfgBitToSet iTmp1 \n"
    "  .set newCfg iTmp2 \n"
    "  lsu0.ldil cfgMask 0xFF80 || lsu1.ldih cfgMask 0xFFFF || cmu.cpti p_cfg_save P_CFG \n"
    "  iau.and newCfg cfgMask p_cfg_save ; Clear all set-able bits \n"
    "  lsu1.ldil cfgBitToSet 0x18 ; Set the bits for rounding \n"
    "  iau.or newCfg cfgBitToSet newCfg \n"
    "  cmu.cpit P_CFG newCfg \n"
    "  .unset cfgMask \n"
    "  .unset cfgBitToSet \n"
    "  .unset newCfg \n"
    ";] \n"
    "\n"
    "cmu.cpsvr.x32 fillColor_f32x4 invalidValue \n"
    "\n"
    "; Set all the four-width SIMD constants \n"
    //"lsu0.ldsl sTmp0 0x0 || lsu1.ldsh sTmp0 0x3f80 ; 1.0f in hex\n"
    //"cmu.cpsvr.x32 ones_f32x4 sTmp0 \n"
    "lsu0.ldil iTmp0 vOnes_f32x4 || lsu1.ldih iTmp0 vOnes_f32x4 \n"
    "lsu0.ldo64.l ones_f32x4 iTmp0 0  || lsu1.ldo64.h ones_f32x4 iTmp0 8 \n"
    "cmu.cpsvr.x32 xReferenceMax_i32x4 xReferenceMax \n"
    "cmu.cpsvr.x32 yReferenceMax_i32x4 yReferenceMax \n"
    "lsu0.ldil addressIncrement 16 || lsu1.ldih addressIncrement 0 \n"
    "\n"
    "iau.sub x x x \n"
    "\n"
    "__mainLoop: \n"
    "lsu0.ldxvi xCoordinates_f32x4 xCoordinates_address addressIncrement || lsu1.ldxvi yCoordinates_f32x4 yCoordinates_address addressIncrement \n"
    "nop 6 \n"
    "lsu0.stxvi ones_f32x4 pOut_address addressIncrement \n"
    //"cmu.cpvv.f32.i32s x0_i32x4 xCoordinates_f32x4 \n"
    //"cmu.cpvv.f32.i32s y0_i32x4 yCoordinates_f32x4 \n"
    //"vau.add.i32s x1_i32x4 x0_i32x4 ones_i32x4 \n"
    //"vau.add.i32s y1_i32x4 y0_i32x4 ones_i32x4 \n"

    //"\n"
    //"; Compute the addresses in the reference image \n"
    //";[ \n"
    //"  ; Naive approach: \n"
    //"  ; const f32 top = (1-x)*pTL + x*pTR; \n"
    //"  ; const f32 bot = (1-x)*pBL + x*pBR; \n"
    //"  ; out = (1-y)*top + y*bot; \n"
    //"\n"
    //"  ; Accumulator approach (just multiply out and substitute all terms): \n"
    //"  ; out = pTL - x*pTL + x*pTR - y*pTL - y*x*pTL + y*x*pTR + y*pBL-y*x*pBL + y*x*pBR \n"
    //"\n"
    //"  .set interpolatedTop_f32x4 vTmp0 \n"
    //"  .set interpolatedBottom_f32x4 vTmp1 \n"
    //"   \n"
    //"   \n"
    //"   \n"
    //"   \n"
    //"   \n"
    //"  .unset interpolatedTop_f32x4 \n"
    //"  .unset interpolatedBottom_f32x4 \n"
    //";] \n"

    //"\n"
    //"; if((x0 <= 0) && (y0 <= 0) && (xReferenceMax <= x1) && (yReferenceMax <= y1) ) then the coordinate is out-of-bounds \n"
    //"cmu.cmz.i32 x0_i32x4 \n"
    //"peu.andacc || cmu.cmz.i32 y0_i32x4 \n"
    //"peu.andacc || cmu.cmvv.i32 xReferenceMax_i32x4 x1_i32x4 \n"
    //"peu.andacc || cmu.cmvv.i32 yReferenceMax_i32x4 y1_i32x4 \n"
    //"peu.pvv32 lte || cmu.cpvv out_f32x4 fillColor_f32x4 \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    //" \n"
    "iau.add x x 4 \n"
    "cmu.cmii.i32 x numCoordinates \n"
    "peu.pc1c lt || bru.bra __mainLoop \n"
    "nop 5 \n"

    //IAU.OR iTMP2, iTMP1, iTMP2

    //"IAU.AND iTMP2, iTMP0, p_cfg_save

    //LSU0.LDIL first_step, 1          || LSU1.LDIL iTMP1, 0x18
    //LSU0.LDIL iTMP0, 0xFFE7          || LSU1.LDIH iTMP0, 0xFFFF          || CMU.CPTI p_cfg_save, P_CFG
    //LSU0.LDIL cmu0_gt, C_CMU0_GT     || LSU1.LDIH cmu0_gt, C_CMU0_GT     || IAU.AND iTMP2, iTMP0, p_cfg_save ; Clear all rounding bits

    //"lsu0.ldi32 curX xCoordinates_address || lsu1.ldi32 curY yCoordinates_address \n"
    //"nop 5 \n"
    //"cmu.cpsi.f32.i32s x0 curX \n"
    //"cmu.cpsi.f32.i32s y0 curY \n"
    //"iau.add x1 x0 1 \n"
    //"iau.add y1 y0 1 \n"

    //"cmu.cmii.i32 x numPixelsToProcess "
    //"peu.pc1c lt || bru.bra _filteringLoop \n"

    //"cmu.cmii.i32 x0 zero \n"

    //"cmu.clamp.i32 x0 xReferenceMax \n"
    //"cmu.clamp.i32 x0 xReferenceMax \n"

    //"peu.oracc || \n"

    //if(x0 < 0.0f || x1 > xReferenceMax || y0 < 0.0f || y1 > yReferenceMax) {
    //  pOut[x] = invalidValue;
    //  continue;
    //}

    //"vau.alignvec integralImage_01 integralImage_01a integralImage_01b 4 \n"
    //  "vau.alignvec integralImage_11 integralImage_11a integralImage_11b 4 \n"
    //  "nop 1 \n"
    //  "vau.sub.i32 output integralImage_11 integralImage_10\n" //output[x] = integralImage_11[x] - integralImage_10[x] + integralImage_00[x] - integralImage_01[x];
    //  "nop 1 \n"
    //  "vau.add.i32 output output integralImage_00 \n"
    //  "nop 1 \n"
    //  "vau.sub.i32 output output integralImage_01 \n"
    //  "nop 1 \n"
    //  "lsu0.stxv output output_address \n"
    //  "iau.add output_address output_address 16 \n \n"

    //  "cmu.cpit P_CFG p_cfg_save ; reset the rounding mode to its value at the start of this function \n"

    : //Output registers
  :"r"(invalidValue), "r"(numCoordinates), "r"(pXCoordinates), "r"(pYCoordinates), "r"(pOut), "r"(xReferenceMax), "r"(yReferenceMax) //Input registers
    :"i5", "i6", "i7", "i10", "i11", "i12", "i13", "s5", "s6"  //Clobbered registers
    );

#endif // INNER_LOOP_VERSION
}