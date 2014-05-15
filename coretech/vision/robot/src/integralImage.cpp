/**
File: integralImage.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/integralImage.h"

#include "anki/common/robot/benchmarking.h"

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
      const s32 imageWidth = image.get_size(1);

      const s32 integralImageHeight = this->get_size(0);
      const s32 integralImageWidth = this->get_size(1);

      //AnkiAssert(imageWidth%ANKI_VISION_IMAGE_WIDTH_SHIFT == 0);
      AnkiAssert(this->imageWidth == imageWidth);

      AnkiConditionalErrorAndReturnValue(numRowsToScroll > 0 && numRowsToScroll <= integralImageHeight,
        RESULT_FAIL_INVALID_PARAMETER, "ScrollingIntegralImage_u8_s32::ScrollDown", "numRowsToScroll is to high or low");

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

          if((lastResult = PadImageRow(image, curImageY, paddedRow)) != RESULT_OK)
            return lastResult;

          ComputeIntegralImageRow(pPaddedRow, this->Pointer(curIntegralImageY-1, 0), this->Pointer(curIntegralImageY, 0), integralImageWidth);

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
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
      const s32 imageWidth4 = imageWidth >> 2;

      AnkiAssert(image.IsValid());
      AnkiAssert(paddedRow.IsValid());
      AnkiAssert(paddedRow.get_size(1) == (imageWidth+2*numBorderPixels));
      AnkiAssert(paddedRow.get_size(0) == 1);
      AnkiAssert(whichRow >= 0);
      AnkiAssert(whichRow < imageHeight);

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
      const s32 integralImageWidth4 = (integralImageWidth+3) / 4;

      const u32 * restrict paddedImage_currentRowU32 = reinterpret_cast<const u32*>(paddedImage_currentRow);

      s32 horizontalSum = 0;
      for(s32 x=0; x<integralImageWidth4; x++) {
        const u32 curPixel = paddedImage_currentRowU32[x];

        horizontalSum += curPixel & 0xFF;
        integralImage_currentRow[4*x] = horizontalSum;

        horizontalSum += (curPixel & 0xFF00) >> 8;
        integralImage_currentRow[4*x + 1] = horizontalSum;

        horizontalSum += (curPixel & 0xFF0000) >> 16;
        integralImage_currentRow[4*x + 2] = horizontalSum;

        horizontalSum += (curPixel & 0xFF000000) >> 24;
        integralImage_currentRow[4*x + 3] = horizontalSum;
      }
    }

    void ScrollingIntegralImage_u8_s32::ComputeIntegralImageRow(const u8 * restrict paddedImage_currentRow, const s32 * restrict integralImage_previousRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth)
    {
      const s32 integralImageWidth4 = (integralImageWidth+3) / 4;

      const u32 * restrict paddedImage_currentRowU32 = reinterpret_cast<const u32*>(paddedImage_currentRow);

      s32 horizontalSum = 0;
      //for(s32 x=0; x<integralImageWidth; x++) {
      //  horizontalSum += paddedImage_currentRow[x];
      //  integralImage_currentRow[x] = horizontalSum + integralImage_previousRow[x];
      //}

      for(s32 x=0; x<integralImageWidth4; x++) {
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
    }
  } // namespace Embedded
} //namespace Anki
