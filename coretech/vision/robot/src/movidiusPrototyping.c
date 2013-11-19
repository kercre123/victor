/**
File: movidiusPrototyping.c
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#undef __cplusplus
#include "anki/common/robot/cInterfaces_c.h"
#include "anki/common/robot/utilities_c.h"

#include "anki/vision/robot/movidiusPrototyping.h"

#ifdef _MSC_VER
#include <emmintrin.h>
#define int4 __m128i
#endif

void ScrollingIntegralImage_u8_s32_FilterRow_innerLoop(const s32 * restrict integralImage_00, const s32 * restrict integralImage_01, const s32 * restrict integralImage_10, const s32 * restrict integralImage_11, const s32 minX, const s32 maxX, const s32 imageWidth, s32 * restrict output)
{
  s32 x;

  if(minX > 0)
    memset(output, 0, minX*sizeof(s32));

  for(x=minX; x<=maxX; x++) {
    output[x] = integralImage_11[x] - integralImage_10[x] + integralImage_00[x] - integralImage_01[x];
  }

  if((maxX+1) < imageWidth)
    memset(output+maxX+1, 0, (imageWidth - (maxX+1))*sizeof(s32));
}

void ScrollingIntegralImage_u8_s32_FilterRow_shaveInnerLoop(const s32 * restrict integralImage_00, const s32 * restrict integralImage_01, const s32 * restrict integralImage_10, const s32 * restrict integralImage_11, const s32 minXSimd, const s32 maxXSimd, s32 * restrict output)
{
#ifndef USING_MOVIDIUS_SHAVE_COMPILER
  assert(0);
#else

#endif // #ifndef USING_MOVIDIUS_SHAVE_COMPILER ... #else
  //s32 x;

  //const int4 * restrict integralImageX4_00  = &(integralImage_00[0]);

  //const int4 * restrict integralImageX4_10  = &(integralImage_10[0]);

  //const int4 * restrict integralImageX4_01a = &(integralImage_01[-1]);
  //const int4 * restrict integralImageX4_01b = &(integralImage_01[3]);

  //const int4 * restrict integralImageX4_11a = &(integralImage_11[-1]);
  //const int4 * restrict integralImageX4_11b = &(integralImage_11[3]);

  //int4 * restrict output = &(output[0]);

  //__asm(
  //".set integralImage_00_address i20 \n"
  //  ".set integralImage_10_address i21 \n"
  //  ".set integralImage_01a_address i22 \n"
  //  ".set integralImage_01b_address i23 \n"
  //  ".set integralImage_11a_address i24 \n"
  //  ".set integralImage_11b_address i25 \n"
  //  ".set output_address i26 \n"
  //  "nop 5 \n"
  //  "cmu.cpii integralImage_00_address %0 \n"
  //  "cmu.cpii integralImage_10_address %1 \n"
  //  "cmu.cpii integralImage_01a_address %2 \n"
  //  "cmu.cpii integralImage_01b_address %3 \n"
  //  "cmu.cpii integralImage_11a_address %4 \n"
  //  "cmu.cpii integralImage_11b_address %5 \n"
  //  "cmu.cpii output_address %6 \n"
  //  : //Output registers
  //:"r"(&integralImageX4_00[0]), "r"(&integralImageX4_10[0]), "r"(&integralImageX4_01a[0]), "r"(&integralImageX4_01b[0]), "r"(&integralImageX4_11a[0]), "r"(&integralImageX4_11b[0]), "r"(&output[0]), //Input registers
  //  :"i20", "i21", "i22", "i23", "i24", "i25", "i26" //Clobbered registers
  //  );

  ////im = zeros([64,64], 'uint8');
  ////for y = 1:64
  ////  im(y, :) = 1 + (y * (1:64)) / 12;
  ////end

  ////ii = integralimage(double(im));

  //for(x=minXSimd; x<maxXSimd; x+=4) {
  //  __asm(
  //  ".set integralImage_00 v0 \n"
  //    ".set integralImage_10 v1 \n"
  //    ".set integralImage_01 v2 \n"
  //    ".set integralImage_01a v3 \n"
  //    ".set integralImage_01b v4 \n"
  //    ".set integralImage_11 v5 \n"
  //    ".set integralImage_11a v6 \n"
  //    ".set integralImage_11b v7 \n"
  //    ".set output v8 \n"
  //    "lsu0.ldxv integralImage_00 integralImage_00_address || lsu1.ldxv integralImage_10 integralImage_10_address\n"
  //    "nop 1 \n"
  //    "lsu0.ldxv integralImage_01a integralImage_01a_address || lsu1.ldxv integralImage_01b integralImage_01b_address\n"
  //    "nop 1 \n"
  //    "lsu0.ldxv integralImage_11a integralImage_11a_address || lsu1.ldxv integralImage_11b integralImage_11b_address\n"
  //    "nop 1 \n"
  //    "iau.add integralImage_00_address integralImage_00_address 16 \n"
  //    "iau.add integralImage_10_address integralImage_10_address 16 \n"
  //    "iau.add integralImage_01a_address integralImage_01a_address 16 \n"
  //    "iau.add integralImage_01b_address integralImage_01b_address 16 \n"
  //    "iau.add integralImage_11a_address integralImage_11a_address 16 \n"
  //    "iau.add integralImage_11b_address integralImage_11b_address 16 \n"
  //    "nop 1 \n"
  //    "vau.alignvec integralImage_01 integralImage_01a integralImage_01b 4 \n"
  //    "vau.alignvec integralImage_11 integralImage_11a integralImage_11b 4 \n"
  //    "nop 1 \n"
  //    "vau.sub output integralImage_11 integralImage_10\n" //output[x] = integralImage_11[x] - integralImage_10[x] + integralImage_00[x] - integralImage_01[x];
  //    "nop 1 \n"
  //    "vau.add output output integralImage_00 \n"
  //    "nop 1 \n"
  //    "vau.sub output output integralImage_01 \n"
  //    "nop 1 \n"
  //    "lsu0.stvx output output_address \n"
  //    "iau.add output_address output_address 16 \n \n"
  //    : //Output registers
  //  ://Input registers
  //  :"i20", "i21", "i22", "i23", "i24", "i25", "i26", "v0", "v1", "v2", "v3", "v4", "v5", "v6", "v7", "v8" //Clobbered registers
  //    );
  //}
}

void ScrollingIntegralImage_u8_s32_FilterRow_innerLoop_emulateShave(const s32 * restrict integralImage_00, const s32 * restrict integralImage_01, const s32 * restrict integralImage_10, const s32 * restrict integralImage_11, const s32 minX, const s32 maxX, const s32 imageWidth, s32 * restrict output)
{
  s32 x;

  const size_t sizet_integralImage_00 = (size_t) integralImage_00;
  const size_t sizet_integralImage_01 = (size_t) integralImage_01;
  const size_t sizet_integralImage_10 = (size_t) integralImage_10;
  const size_t sizet_integralImage_11 = (size_t) integralImage_11;
  const size_t sizet_output = (size_t) output;

  assert(minX <= maxX);
  assert(imageWidth >= maxX);
  assert((imageWidth % 4) == 0);

  // This version only works with filter that are 8-byte aligned
  assert((sizet_integralImage_00 % 8) == 0);
  assert((sizet_integralImage_10 % 8) == 0);

  // WARNING: The Shaves will require this alignment, so
  // TODO: fix it for the unit tests
  // This version will only work with filters that have (width+1) being a multiple of 8
  // This means that if the left corners are 8-bytes aligned, the right ones are as well
  //assert((sizet_integralImage_01 - sizet_integralImage_00) % 8 == 0);
  //assert((sizet_integralImage_11 - sizet_integralImage_10) % 8 == 0);

  // I think these should be true for most C compilers, but just checking
  assert((sizet_integralImage_00 % 4) == 0);
  assert((sizet_integralImage_01 % 4) == 0);
  assert((sizet_integralImage_10 % 4) == 0);
  assert((sizet_integralImage_11 % 4) == 0);

  // Primary loop
  {
    // The address must be divisible by 8 (8-byte aligned)
    // byteAlignedOffset is the index of the first s32 that is 8-bytes aligned. It should be either 0 or 4.
    const size_t byteAlignedOffset_00 = (((1 + ((sizet_integralImage_00 - 1) >> 3)) << 3) - sizet_integralImage_00);

    // The simd load must be 8-byte aligned.
    const s32 minXSimd = -((s32)byteAlignedOffset_00 >> 2) + ((minX >> 2) << 2);

    // We must process a 4-divisible number of elements
    const s32 maxXSimd = minXSimd + ((1 + ((maxX-minXSimd) >> 2)) << 2);

    assert(((minXSimd+(s32)sizet_output) % 8) == 0);

    assert(minXSimd >= 0);
    assert((maxXSimd-1) < (imageWidth+4));

    assert(byteAlignedOffset_00 == 0 || byteAlignedOffset_00 == 4);

    // All these should be true, and might also be checked by some other asserts
    {
      const size_t byteAlignedOffset_01 = (((1 + ((sizet_integralImage_01 - 1) >> 3)) << 3) - sizet_integralImage_01);
      const size_t byteAlignedOffset_10 = (((1 + ((sizet_integralImage_10 - 1) >> 3)) << 3) - sizet_integralImage_10);
      const size_t byteAlignedOffset_11 = (((1 + ((sizet_integralImage_11 - 1) >> 3)) << 3) - sizet_integralImage_11);

      if(byteAlignedOffset_00 == 0) {
        assert(byteAlignedOffset_01 == 4);
        assert(byteAlignedOffset_10 == 0);
        assert(byteAlignedOffset_11 == 4);
      } else if(byteAlignedOffset_00 == 4) {
        assert(byteAlignedOffset_01 == 0);
        assert(byteAlignedOffset_10 == 4);
        assert(byteAlignedOffset_11 == 0);
      } else {
        assert(0);
      }
    }

    // This is the actual processing loop
    for(x=minXSimd; x<maxXSimd; x+=4) {
      output[x] = integralImage_11[x] - integralImage_10[x] + integralImage_00[x] - integralImage_01[x];
      output[x+1] = integralImage_11[x+1] - integralImage_10[x+1] + integralImage_00[x+1] - integralImage_01[x+1];
      output[x+2] = integralImage_11[x+2] - integralImage_10[x+2] + integralImage_00[x+2] - integralImage_01[x+2];
      output[x+3] = integralImage_11[x+3] - integralImage_10[x+3] + integralImage_00[x+3] - integralImage_01[x+3];
    }
  } // end Primary loop

  // Black out invalid areas
  if(minX > 0)
    memset(output, 0, minX*sizeof(s32));

  if((maxX+1) < imageWidth)
    memset(output+maxX+1, 0, (imageWidth - (maxX+1))*sizeof(s32));
}

// This might be used later, but not yet
#if 0
s32 ScrollingIntegralImage_u8_s32_FilterRow(
  const C_ScrollingIntegralImage_u8_s32 * restrict integralImage,
  const C_Rectangle_s16 * restrict filter,
  const s32 imageRow,
  C_Array_s32 * restrict output)
{
  const s32 insufficientBorderPixels_left = (-filter->left+1) > integralImage->numBorderPixels;
  const s32 insufficientBorderPixels_right = (filter->right) > integralImage->numBorderPixels;

  // These min and max coordinates are in the original image's coordinate frame
  const s32 minX = insufficientBorderPixels_left ? -(integralImage->numBorderPixels + filter->left - 1) : 0;
  const s32 maxX = insufficientBorderPixels_right ? (integralImage->size1 - 1 - filter->right + integralImage->numBorderPixels) : (integralImage->size1 - 1);

  // Get the four pointers to the corners of the filter in the integral image.
  // The x offset is added at the end, because it might be invalid for x==0.
  // The -1 terms are because the rectangular sums should be inclusive.
  /*const s32 topOffset = imageRow + filter->top - 1 + integralImage->numBorderPixels - integralImage->rowOffset + integralImage->numBorderPixels;
  const s32 bottomOffset = imageRow + filter->bottom + integralImage->numBorderPixels - integralImage->rowOffset + integralImage->numBorderPixels;*/
  const s32 topOffset = imageRow - integralImage->rowOffset + filter->top - 1 ;
  const s32 bottomOffset = imageRow - integralImage->rowOffset + filter->bottom;
  const s32 leftOffset = filter->left - 1 + integralImage->numBorderPixels;
  const s32 rightOffset = filter->right + integralImage->numBorderPixels;

  const s32 * restrict integralImage_00 = ARRAY_S32_POINTER(integralImage->data, integralImage->stride, topOffset, 0) + leftOffset;
  const s32 * restrict integralImage_01 = ARRAY_S32_POINTER(integralImage->data, integralImage->stride, topOffset, 0) + rightOffset;
  const s32 * restrict integralImage_10 = ARRAY_S32_POINTER(integralImage->data, integralImage->stride, bottomOffset, 0) + leftOffset;
  const s32 * restrict integralImage_11 = ARRAY_S32_POINTER(integralImage->data, integralImage->stride, bottomOffset, 0) + rightOffset;

  s32 * restrict pOutput = output->data;

  ScrollingIntegralImage_u8_s32_FilterRow_innerLoop(integralImage_00, integralImage_01, integralImage_10, integralImage_11, minX, maxX, integralImage->imageWidth, pOutput);

  return RESULT_OK;
}
#endif // #if 0