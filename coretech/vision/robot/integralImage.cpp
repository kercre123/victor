/**
File: integralImage.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/
#include "coretech/vision/robot/integralImage.h"
#include "coretech/vision/robot/imageProcessing.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/hostIntrinsics_m4.h"

namespace Anki
{
  namespace Embedded
  {
    IntegralImage_u8_s32::IntegralImage_u8_s32()
      : Array<s32>()
    {
    }

    IntegralImage_u8_s32::IntegralImage_u8_s32(const Array<u8> &image, MemoryStack &memory, const Flags::Buffer flags)
      : Array<s32>(image.get_size(0) + 1, image.get_size(1) + 1, memory, flags)
    {
      AnkiConditionalErrorAndReturn(this->IsValid(),
        "IntegralImage_u8_s32::IntegralImage_u8_s32", "Could not allocate array");

      const s32 integralImageHeight = this->get_size(0);
      const s32 integralImageWidth = this->get_size(1);

      {
        PUSH_MEMORY_STACK(memory);

        memset(this->Pointer(0,0), 0, this->get_stride());

        for(s32 y=1; y<integralImageHeight; y++) {
          const u8 * restrict pImage = image.Pointer(y-1, 0);
          const s32 * restrict pPrevious = this->Pointer(y-1, 0);

          s32 * restrict pCurrent  = this->Pointer(y, 0);

          s32 horizontalSum = 0;

          pCurrent[0] = 0;
          for(s32 x=1; x<integralImageWidth; x++) {
            horizontalSum += pImage[x-1];
            pCurrent[x] = horizontalSum + pPrevious[x];
          }
        }
      }
    }

    IntegralImage_u8_u16::IntegralImage_u8_u16()
      : Array<u16>()
    {
    }

    IntegralImage_u8_u16::IntegralImage_u8_u16(const Array<u8> &image, MemoryStack &memory, const Flags::Buffer flags)
      : Array<u16>(image.get_size(0) + 1, image.get_size(1) + 1, memory, flags)
    {
      AnkiConditionalErrorAndReturn(this->IsValid(),
        "IntegralImage_u8_u16::IntegralImage_u8_u16", "Could not allocate array");

      const u16 integralImageHeight = this->get_size(0);
      const u16 integralImageWidth = this->get_size(1);

      {
        PUSH_MEMORY_STACK(memory);

        memset(this->Pointer(0,0), 0, this->get_stride());

        for(u16 y=1; y<integralImageHeight; y++) {
          const u8 * restrict pImage = image.Pointer(y-1, 0);
          const u16 * restrict pPrevious = this->Pointer(y-1, 0);

          u16 * restrict pCurrent  = this->Pointer(y, 0);

          u16 horizontalSum = 0;

          pCurrent[0] = 0;
          for(u16 x=1; x<integralImageWidth; x++) {
            horizontalSum += pImage[x-1];
            pCurrent[x] = horizontalSum + pPrevious[x];
          }
        }
      }
    }

    ScrollingIntegralImage_u8_s32::ScrollingIntegralImage_u8_s32()
      : Array<s32>(), imageWidth(-1), maxRow(-1), rowOffset(-1), numBorderPixels(-1)
    {
    }

    ScrollingIntegralImage_u8_s32::ScrollingIntegralImage_u8_s32(const s32 bufferHeight, const s32 imageWidth, const s32 numBorderPixels, MemoryStack &memory, const Flags::Buffer flags)
      : Array<s32>(bufferHeight, imageWidth+2*numBorderPixels, memory, flags), imageWidth(imageWidth), maxRow(-1), rowOffset(-numBorderPixels), numBorderPixels(numBorderPixels)
    {
      //AnkiAssert(imageWidth%ANKI_VISION_IMAGE_WIDTH_SHIFT == 0);

      if(numBorderPixels < 0 || numBorderPixels > bufferHeight || numBorderPixels > imageWidth) {
        AnkiError("Anki.ScrollingIntegralImage_u8_s32.ScrollingIntegralImage_u8_s32", "numBorderPixels must be greater than or equal to zero, and less than the size of this ScrollingIntegralImage.");
        InvalidateArray();
        return;
      }
    }

    Result ScrollingIntegralImage_u8_s32::ScrollDown(const Array<u8> &image, s32 numRowsToScroll, MemoryStack scratch)
    {
      Result lastResult;

      const s32 imageHeight = image.get_size(0);

      const s32 integralImageHeight = this->get_size(0);
      const s32 integralImageWidth = this->get_size(1);

      //AnkiAssert(imageWidth%ANKI_VISION_IMAGE_WIDTH_SHIFT == 0);
      AnkiAssert(this->imageWidth == image.get_size(1));

      AnkiConditionalErrorAndReturnValue(numRowsToScroll > 0 && numRowsToScroll <= integralImageHeight,
                                         RESULT_FAIL_INVALID_PARAMETER, "ScrollingIntegralImage_u8_s32::ScrollDown",
                                         "numRowsToScroll is too high or low: %d (intImgHeight: %d)",
                                         numRowsToScroll, integralImageHeight);

      Array<u8> paddedRow(1, integralImageWidth, scratch);
      u8 * restrict pPaddedRow = paddedRow.Pointer(0, 0);

      s32 curIntegralImageY;
      s32 curImageY;

      // If we are asked to scroll all rows, we won't keep track of any of the previous data, so
      // we'll start by initializing the first rows
      if(numRowsToScroll == integralImageHeight) {
        BeginBenchmark("scrolling_first");

        if((lastResult = PadImageRow(image, 0, paddedRow)) != RESULT_OK)
          return lastResult;

        curIntegralImageY = 0;

        // Create the first line
        ComputeIntegralImageRow(pPaddedRow, this->Pointer(0, 0), integralImageWidth);
        curIntegralImageY++;

        // Fill in the top padded rows
        if(numBorderPixels > 0) {
          //for curIntegralImageY = 2:(1+extraBorderWidth)
          for(s32 y=0; y<numBorderPixels; y++) {
            ComputeIntegralImageRow(pPaddedRow, this->Pointer(curIntegralImageY-1, 0), this->Pointer(curIntegralImageY, 0), integralImageWidth);
            curIntegralImageY++;
          }

          numRowsToScroll -= numBorderPixels;
        } // if(numBorderPixels == 0)

        numRowsToScroll--;
        curImageY = 0;

        EndBenchmark("scrolling_first");
      } else { // if(numRowsToScroll == integralImageHeight)
        BeginBenchmark("scrolling_nth");

        curImageY = this->maxRow;
        curIntegralImageY = integralImageHeight - numRowsToScroll;

        // Scroll this integral image up
        // integralImage(1:(end-numRowsToUpdate), :) = integralImage((1+numRowsToUpdate):end, :);
        for(s32 y=0; y<(integralImageHeight-numRowsToScroll); y++) {
          s32 * restrict pIntegralImage_yDst = this->Pointer(y, 0);
          const s32 * restrict pIntegralImage_ySrc = this->Pointer(y+numRowsToScroll, 0);

          memcpy(pIntegralImage_yDst, pIntegralImage_ySrc, this->get_stride());
        }
        this->rowOffset += numRowsToScroll;

        EndBenchmark("scrolling_nth");
      } // if(numRowsToScroll == integralImageHeight) ... else

      // Compute the non-padded integral image rows
      if(curImageY < imageHeight) {
        BeginBenchmark("scrolling_nonPadded");

        //for iy = 1:numRowsToScroll
        while(numRowsToScroll > 0) {
          curImageY++;

          // If we've run out of image, exit this loop, then start the bottom-padding loop
          if(curImageY >= imageHeight) {
            curImageY = imageHeight - 1;
            break;
          }

          //BeginBenchmark("scrolling_nonPadded_pad");
          if((lastResult = PadImageRow(image, curImageY, paddedRow)) != RESULT_OK)
            return lastResult;
          //EndBenchmark("scrolling_nonPadded_pad");

          //BeginBenchmark("scrolling_nonPadded_compute");
          ComputeIntegralImageRow(pPaddedRow, this->Pointer(curIntegralImageY-1, 0), this->Pointer(curIntegralImageY, 0), integralImageWidth);
          //EndBenchmark("scrolling_nonPadded_compute");

          numRowsToScroll--;
          curIntegralImageY++;
        }

        EndBenchmark("scrolling_nonPadded");
      } else {
        curImageY = imageHeight - 1;
      }

      this->maxRow = curImageY;
      //this->minRow = MAX(0, this->maxRow-integralImageHeight+1);

      // If we're at the bottom of the image, compute the extra bottom padded rows
      if(numRowsToScroll > 0) {
        BeginBenchmark("scrolling_scroll");

        //paddedImageRow = padImageRow(image, curImageY, numBorderPixels);
        if((lastResult = PadImageRow(image, curImageY, paddedRow)) != RESULT_OK)
          return lastResult;

        //for iy = (iy+1):numRowsToScroll
        while(numRowsToScroll > 0) {
          // integralImage(curIntegralImageY,1) = paddedImageRow(1,1) + integralImage(curIntegralImageY-1,1);
          // for x = 2:size(paddedImageRow, 2)
          //     integralImage(curIntegralImageY,x) = paddedImageRow(1,x) + integralImage(curIntegralImageY,x-1) + integralImage(curIntegralImageY-1,x) - integralImage(curIntegralImageY-1,x-1);
          // end
          ComputeIntegralImageRow(pPaddedRow, this->Pointer(curIntegralImageY-1, 0), this->Pointer(curIntegralImageY, 0), integralImageWidth);

          curIntegralImageY++;
          numRowsToScroll--;
          //this->minRow++;
        }

        EndBenchmark("scrolling_scroll");
      }

      return RESULT_OK;
    }

    Result ScrollingIntegralImage_u8_s32::FilterRow(const Rectangle<s16> &filter, const s32 imageRow, Array<s32> &output) const
    {
      return this->FilterRow<s32>(filter, imageRow, 1, 0, output);
    }

    // TODO: implement
    //s32 ScrollingIntegralImage_u8_s32::get_minRow(const s32 filterHalfHeight) const
    //{
    //  AnkiAssert(filterHalfHeight >= 0);

    //  //const s32 shiftedMinRow = minRow + filterHalfHeight + this->rowOffset + 1;
    //  const s32 shiftedMinRow = minRow + filterHalfHeight - 1;

    //  return shiftedMinRow;
    //}

    s32 ScrollingIntegralImage_u8_s32::get_maxRow(const s32 filterHalfHeight) const
    {
      AnkiAssert(filterHalfHeight >= 0);

      // TODO: make cleaner ?

      s32 shiftedMaxRow = maxRow - filterHalfHeight;
      const s32 bottomOffset = shiftedMaxRow - this->rowOffset + filterHalfHeight;

      if(bottomOffset < (this->size[0]-1)) {
        shiftedMaxRow += (this->size[0]-1) - bottomOffset;
      }

      return shiftedMaxRow;
    }

    s32 ScrollingIntegralImage_u8_s32::get_imageWidth() const
    {
      return imageWidth;
    }

    s32 ScrollingIntegralImage_u8_s32::get_numBorderPixels() const
    {
      return numBorderPixels;
    }

    s32 ScrollingIntegralImage_u8_s32::get_rowOffset() const
    {
      return rowOffset;
    }

    Result ScrollingIntegralImage_u8_s32::PadImageRow(const Array<u8> &image, const s32 whichRow, Array<u8> &paddedRow)
    {
      const s32 imageWidth = image.get_size(1);
      const s32 imageWidth4 = imageWidth >> 2;

      AnkiAssert(image.IsValid());
      AnkiAssert(paddedRow.IsValid());
      AnkiAssert(AreEqualSize(1, imageWidth+2*numBorderPixels, paddedRow));
      AnkiAssert(whichRow >= 0);
      AnkiAssert(whichRow < image.get_size(0)); // check row against image height

      const u32 * restrict image_u32rowPointer = reinterpret_cast<const u32*>(image.Pointer(whichRow, 0));
      const u8 * restrict image_u8rowPointer = image.Pointer(whichRow, 0);

      // Technically aliases, but the writes don't overlap
      u8 * restrict paddedRow_u8rowPointer = paddedRow.Pointer(0, 0);
      u32 * restrict paddedRow_u32rowPointerPxpad = reinterpret_cast<u32*>(paddedRow_u8rowPointer + numBorderPixels);

      const u8 firstPixel = image_u8rowPointer[0];
      const u8 lastPixel = image_u8rowPointer[imageWidth-1];

      s32 xPad = 0;
      for(s32 x=0; x<numBorderPixels; x++) {
        paddedRow_u8rowPointer[xPad++] = firstPixel;
      }

      for(s32 xImage = 0; xImage<imageWidth4; xImage++) {
        paddedRow_u32rowPointerPxpad[xImage] = image_u32rowPointer[xImage];
      }

      xPad += imageWidth;
      for(s32 x=0; x<numBorderPixels; x++) {
        paddedRow_u8rowPointer[xPad++] = lastPixel;
      }

      return RESULT_OK;
    }

    void ScrollingIntegralImage_u8_s32::ComputeIntegralImageRow(const u8* restrict paddedImage_currentRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth)
    {
      s32 horizontalSum = 0;

      s32 x = 0;
      for(; x<integralImageWidth; x++) {
        const u8 curPixel = paddedImage_currentRow[x];

        horizontalSum += curPixel;
        integralImage_currentRow[x] = horizontalSum;
      }
    }

    void ScrollingIntegralImage_u8_s32::ComputeIntegralImageRow(const u8 * restrict paddedImage_currentRow, const s32 * restrict integralImage_previousRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth)
    {
      u32 horizontalSum = 0;

      s32 x = 0;

#if ACCELERATION_TYPE == ACCELERATION_ARM_M4

      const s32 integralImageWidth4 = (integralImageWidth-3) / 4;

      const u32 * restrict paddedImage_currentRowU32 = reinterpret_cast<const u32*>(paddedImage_currentRow);

      for(; x<integralImageWidth4; x++) {
        const u32 curPixel = paddedImage_currentRowU32[x];
 
        horizontalSum += curPixel & 0xFF;
        integralImage_currentRow[4*x] = horizontalSum + integralImage_previousRow[4*x];
 
        horizontalSum += (curPixel & 0xFF00) >> 8;
        integralImage_currentRow[4*x + 1] = horizontalSum + integralImage_previousRow[4*x + 1];
 
        horizontalSum += (curPixel & 0xFF0000) >> 16;
        integralImage_currentRow[4*x + 2] = horizontalSum + integralImage_previousRow[4*x + 2];
 
        horizontalSum += (curPixel & 0xFF000000) >> 24;
        integralImage_currentRow[4*x + 3] = horizontalSum + integralImage_previousRow[4*x + 3];
      }

      x *= 4;

// Initial attempt at neon optimization is no better than non-accelerated version below
// #elif ACCELERATION_TYPE == ACCELERATION_ARM_A7

//       int32x4_t kZeros = vdupq_n_s32(0);
//       for(; x < integralImageWidth; x += 8)
//       {
//         uint8x8_t paddedImageRow = vld1_u8(paddedImage_currentRow);
//         paddedImage_currentRow += 8;

//         uint16x8_t row16x8 = vmovl_u8(paddedImageRow);
//         uint16x4_t row16x4_1 = vget_low_u16(row16x8);
//         uint16x4_t row16x4_2 = vget_high_u16(row16x8);
//         uint32x4_t row32x4_1 = vmovl_u16(row16x4_1);
//         uint32x4_t row32x4_2 = vmovl_u16(row16x4_2);

//         uint32x4_t row1Shift = vextq_u32(kZeros, row32x4_1, 3);
//         row32x4_1 = vaddq_u32(row32x4_1, row1Shift);

//         row1Shift = vextq_u32(kZeros, row1Shift, 3);
//         row32x4_1 = vaddq_u32(row32x4_1, row1Shift);

//         row1Shift = vextq_u32(kZeros, row1Shift, 3);
//         row32x4_1 = vaddq_u32(row32x4_1, row1Shift);

//         uint32x4_t sum1 = vdupq_n_u32(horizontalSum);
//         sum1 = vaddq_u32(row32x4_1, sum1);
        
//         uint32_t sum = vgetq_lane_u32(row32x4_1, 3);
//         uint32x4_t sumDup = vdupq_n_u32(sum);

//         uint32x4_t row2Shift = vextq_u32(kZeros, row32x4_2, 3);
//         row32x4_2 = vaddq_u32(row32x4_2, row2Shift);

//         row2Shift = vextq_u32(kZeros, row2Shift, 3);
//         row32x4_2 = vaddq_u32(row32x4_2, row2Shift);

//         row2Shift = vextq_u32(kZeros, row2Shift, 3);
//         row32x4_2 = vaddq_u32(row32x4_2, row2Shift);

//         row32x4_2 = vaddq_u32(row32x4_2, sumDup);

//         uint32x4_t sum2 = vdupq_n_u32(horizontalSum);
//         sum2 = vaddq_u32(row32x4_2, sum2);

//         int32x4_t previousRow0 = vld1q_s32(integralImage_previousRow);
//         integralImage_previousRow += 4;

//         previousRow0 = vaddq_s32((int32x4_t)sum1, previousRow0);
//         vst1q_s32(integralImage_currentRow, previousRow0);
//         integralImage_currentRow += 4;

//         int32x4_t previousRow1 = vld1q_s32(integralImage_previousRow);
//         integralImage_previousRow += 4;

//         previousRow1 = vaddq_s32((int32x4_t)sum2, previousRow1);
//         vst1q_s32(integralImage_currentRow, previousRow1);
//         integralImage_currentRow += 4;

//         horizontalSum = vgetq_lane_u32(sum2, 3);
//       }

//       if(x >= integralImageWidth)
//       {
//         x -= (8 - 1);
//       }

//       for(; x<integralImageWidth; x++)
//       {
//         horizontalSum += *paddedImage_currentRow;
//         *integralImage_currentRow = horizontalSum + *integralImage_previousRow;
//         paddedImage_currentRow++;
//         integralImage_previousRow++;
//         integralImage_currentRow++;
//       }

#else

      for(; x<integralImageWidth; x++) {
        horizontalSum += paddedImage_currentRow[x];
        integralImage_currentRow[x] = horizontalSum + integralImage_previousRow[x];
      }

#endif
    }

    template<> void ScrollingIntegralImage_u8_s32::FilterRow_innerLoop(
      const s32 minX,
      const s32 maxX,
      const s32 outputMultiply,
      const s32 outputRightShift,
      const s32 * restrict pIntegralImage_00,
      const s32 * restrict pIntegralImage_01,
      const s32 * restrict pIntegralImage_10,
      const s32 * restrict pIntegralImage_11,
      u8 * restrict pOutput)
    {
#if ACCELERATION_TYPE == ACCELERATION_NONE
      
      if(outputMultiply == 1 && outputRightShift == 0) {
        for(s32 x=minX; x<=maxX; x++) {
          pOutput[x] = static_cast<u8>( pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x] );
        }
      } else {
        for(s32 x=minX; x<=maxX; x++) {
          pOutput[x] = static_cast<u8>( ((pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x]) * outputMultiply) >> outputRightShift ) ;
        }
      }

#elif ACCELERATION_TYPE == ACCELERATION_ARM_M4

      if(outputMultiply == 1 && outputRightShift == 0) {
        const s32 maxX_firstCycles = RoundUp(minX, 4);

        for(s32 x=minX; x<maxX_firstCycles; x++) {
          pOutput[x] = static_cast<u8>( pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x] );
        }

        u32 * restrict pOutputU32 = reinterpret_cast<u32*>(pOutput);

        const s32 minX_simd4 = maxX_firstCycles / 4;
        const s32 maxX_simd4 = maxX / 4;
        for(s32 x=minX_simd4; x<=maxX_simd4; x++) {
          const u32 out0 = static_cast<u8>( pIntegralImage_11[x*4] - pIntegralImage_10[x*4] + pIntegralImage_00[x*4] - pIntegralImage_01[x*4] );
          const u32 out1 = static_cast<u8>( pIntegralImage_11[x*4+1] - pIntegralImage_10[x*4+1] + pIntegralImage_00[x*4+1] - pIntegralImage_01[x*4+1] );
          const u32 out2 = static_cast<u8>( pIntegralImage_11[x*4+2] - pIntegralImage_10[x*4+2] + pIntegralImage_00[x*4+2] - pIntegralImage_01[x*4+2] );
          const u32 out3 = static_cast<u8>( pIntegralImage_11[x*4+3] - pIntegralImage_10[x*4+3] + pIntegralImage_00[x*4+3] - pIntegralImage_01[x*4+3] );

          pOutputU32[x] = (out0 & 0xFF) | ((out1 & 0xFF) << 8) | ((out2 & 0xFF) << 16) | ((out3 & 0xFF) << 24);
        }
      } else {
        const s32 maxX_firstCycles = RoundUp(minX, 4);

        for(s32 x=minX; x<maxX_firstCycles; x++) {
          pOutput[x] = static_cast<u8>( ((pIntegralImage_11[x] - pIntegralImage_10[x] + pIntegralImage_00[x] - pIntegralImage_01[x] ) * outputMultiply) >> outputRightShift );
        }

        u32 * restrict pOutputU32 = reinterpret_cast<u32*>(pOutput);

        const s32 minX_simd4 = maxX_firstCycles / 4;
        const s32 maxX_simd4 = maxX / 4;
        for(s32 x=minX_simd4; x<=maxX_simd4; x++) {
          const u32 out0 = static_cast<u8>( ((pIntegralImage_11[x*4] - pIntegralImage_10[x*4] + pIntegralImage_00[x*4] - pIntegralImage_01[x*4]) * outputMultiply) >> outputRightShift );
          const u32 out1 = static_cast<u8>( ((pIntegralImage_11[x*4+1] - pIntegralImage_10[x*4+1] + pIntegralImage_00[x*4+1] - pIntegralImage_01[x*4+1]) * outputMultiply) >> outputRightShift );
          const u32 out2 = static_cast<u8>( ((pIntegralImage_11[x*4+2] - pIntegralImage_10[x*4+2] + pIntegralImage_00[x*4+2] - pIntegralImage_01[x*4+2]) * outputMultiply) >> outputRightShift );
          const u32 out3 = static_cast<u8>( ((pIntegralImage_11[x*4+3] - pIntegralImage_10[x*4+3] + pIntegralImage_00[x*4+3] - pIntegralImage_01[x*4+3]) * outputMultiply) >> outputRightShift );

          pOutputU32[x] = (out0 & 0xFF) | ((out1 & 0xFF) << 8) | ((out2 & 0xFF) << 16) | ((out3 & 0xFF) << 24);
        }
      }

#elif ACCELERATION_TYPE == ACCELERATION_ARM_A7

        // Setup the multiply and shift vectors
      int32x4_t outMult = vdupq_n_s32(outputMultiply);
      int32x4_t outShift = vdupq_n_s32(-outputRightShift); // Negative for right shift

      const u32 kNumElementsProcessedPerLoop = 8;
      const u32 kNumIntegralElementsLoadedAtATime = kNumElementsProcessedPerLoop/2;
      const s32 kMaxXNeon = maxX - (kNumElementsProcessedPerLoop - 1);

      // Process as much as we can with NEON, whatever is left over will need to be done one element at a time
      s32 x = minX;
      for(; x <= kMaxXNeon; x += kNumElementsProcessedPerLoop)
      {
        // Load 4 elements from each pIntegralImage
        int32x4_t img_00 = vld1q_s32(pIntegralImage_00);
        pIntegralImage_00 += kNumIntegralElementsLoadedAtATime;
        int32x4_t img_01 = vld1q_s32(pIntegralImage_01);
        pIntegralImage_01 += kNumIntegralElementsLoadedAtATime;
        int32x4_t img_10 = vld1q_s32(pIntegralImage_10);
        pIntegralImage_10 += kNumIntegralElementsLoadedAtATime;
        int32x4_t img_11 = vld1q_s32(pIntegralImage_11);
        pIntegralImage_11 += kNumIntegralElementsLoadedAtATime;

        // ((11[x] - 10[x] + 00[x] - 01[x]) * outputMultiply) >> outputRightShift 
        int32x4_t out = vsubq_s32(img_11, img_10);
        out = vaddq_s32(out, img_00);
        out = vsubq_s32(out, img_01);
        out = vmulq_s32(out, outMult);
        out = vshlq_s32(out, outShift);

        // Narrow from s32 to s16 as the first step of converting from s32 to u8
        int16x4_t outNar0 = vmovn_s32(out);

        // Repeat the above for the next 4 elements so we will have 8 elements total
        img_00 = vld1q_s32(pIntegralImage_00);
        pIntegralImage_00 += kNumIntegralElementsLoadedAtATime;
        img_01 = vld1q_s32(pIntegralImage_01);
        pIntegralImage_01 += kNumIntegralElementsLoadedAtATime;
        img_10 = vld1q_s32(pIntegralImage_10);
        pIntegralImage_10 += kNumIntegralElementsLoadedAtATime;
        img_11 = vld1q_s32(pIntegralImage_11);
        pIntegralImage_11 += kNumIntegralElementsLoadedAtATime;

        out = vsubq_s32(img_11, img_10);
        out = vaddq_s32(out, img_00);
        out = vsubq_s32(out, img_01);
        out = vmulq_s32(out, outMult);
        out = vshlq_s32(out, outShift);

        int16x4_t outNar1 = vmovn_u32(out);

        // Combine the two s16x4 vectors into an 8 element vector and narrow to s8
        int16x8_t outComb = vcombine_s16(outNar0, outNar1);
        int8x8_t output = vmovn_s16(outComb);

        // Store to pOutput as u8
        vst1_u8(pOutput, (uint8x8_t)output);
        pOutput += kNumElementsProcessedPerLoop;
      }

      for(; x <= maxX; x++)
      {
        *pOutput = static_cast<u8>(((*pIntegralImage_11 - *pIntegralImage_10 + *pIntegralImage_00 - *pIntegralImage_01) * outputMultiply) >> outputRightShift);
        
        pOutput++;
        pIntegralImage_00++;
        pIntegralImage_01++;
        pIntegralImage_10++;
        pIntegralImage_11++;
      }

#else
#error Unknown acceleration
#endif // #if ACCELERATION_TYPE == ACCELERATION_NONE ... #else
    } // FilterRow_innerLoop()
  } // namespace Embedded
} //namespace Anki
