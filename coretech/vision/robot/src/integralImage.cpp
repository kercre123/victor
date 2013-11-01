#include "anki/vision/robot/integralImage.h"

#define SwapEndianU32(value) \
  ((((u32)((value) & 0x000000FF)) << 24) | \
  ( ((u32)((value) & 0x0000FF00)) <<  8) | \
  ( ((u32)((value) & 0x00FF0000)) >>  8) | \
  ( ((u32)((value) & 0xFF000000)) >> 24))

namespace Anki
{
  namespace Embedded
  {
    ScrollingIntegralImage_u8_s32::ScrollingIntegralImage_u8_s32()
      : Array<s32>(), imageWidth(-1), maxRow(-1), rowOffset(-1), numBorderPixels(-1)
    {
    }

    ScrollingIntegralImage_u8_s32::ScrollingIntegralImage_u8_s32(const s32 bufferHeight, const s32 imageWidth, const s32 numBorderPixels, MemoryStack &memory, const bool useBoundaryFillPatterns)
      : Array<s32>(bufferHeight, imageWidth+2*numBorderPixels, memory, useBoundaryFillPatterns), imageWidth(imageWidth), numBorderPixels(numBorderPixels), maxRow(-1), rowOffset(-numBorderPixels)
    {
      assert(imageWidth%ANKI_VISION_IMAGE_WIDTH_SHIFT == 0);

      if(numBorderPixels < 0 || numBorderPixels > bufferHeight || numBorderPixels > imageWidth) {
        AnkiError("Anki.ScrollingIntegralImage_u8_s32.ScrollingIntegralImage_u8_s32", "numBorderPixels must be greater than or equal to zero, and less than the size of this ScrollingIntegralImage.");
        InvalidateArray();
        return;
      }
    }

    Result ScrollingIntegralImage_u8_s32::ScrollDown(const Array<u8> &image, s32 numRowsToScroll, MemoryStack scratch)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      const s32 integralImageHeight = this->get_size(0);
      const s32 integralImageWidth = this->get_size(1);

      assert(imageWidth%ANKI_VISION_IMAGE_WIDTH_SHIFT == 0);
      assert(imageWidth == imageWidth);

      AnkiConditionalErrorAndReturnValue(numRowsToScroll > 0 && numRowsToScroll <= integralImageHeight,
        RESULT_FAIL, "ScrollingIntegralImage_u8_s32::ScrollDown", "numRowsToScroll is to high or low");

      Array<u8> paddedRow(1, integralImageWidth, scratch);
      u8 * restrict pPaddedRow = paddedRow.Pointer(0, 0);

      s32 curIntegralImageY;
      s32 curImageY;

      // If we are asked to scroll all rows, we won't keep track of any of the previous data, so
      // we'll start by initializing the first rows
      if(numRowsToScroll == integralImageHeight) {
        if(PadImageRow(image, 0, paddedRow) != RESULT_OK)
          return RESULT_FAIL;

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
      } else { // if(numRowsToScroll == integralImageHeight)
        curImageY = this->maxRow;
        curIntegralImageY = integralImageHeight - numRowsToScroll;

        // Scroll this integral image up
        // integralImage(1:(end-numRowsToUpdate), :) = integralImage((1+numRowsToUpdate):end, :);
        for(s32 y=0; y<(integralImageHeight-numRowsToScroll); y++) {
          s32 * restrict pIntegralImage_yDst = this->Pointer(y, 0);
          const s32 * restrict pIntegralImage_ySrc = this->Pointer(y+numRowsToScroll, 0);

          memcpy(pIntegralImage_yDst, pIntegralImage_ySrc, this->get_strideWithoutFillPatterns());
        }
        this->rowOffset += numRowsToScroll;
      } // if(numRowsToScroll == integralImageHeight) ... else

      // Compute the non-padded integral image rows
      if(curImageY < imageHeight) {
        //for iy = 1:numRowsToScroll
        while(numRowsToScroll > 0) {
          curImageY++;

          // If we've run out of image, exit this loop, then start the bottom-padding loop
          if(curImageY >= imageHeight) {
            curImageY = imageHeight - 1;
            break;
          }

          if(PadImageRow(image, curImageY, paddedRow) != RESULT_OK)
            return RESULT_FAIL;

          ComputeIntegralImageRow(pPaddedRow, this->Pointer(curIntegralImageY-1, 0), this->Pointer(curIntegralImageY, 0), integralImageWidth);

          numRowsToScroll--;
          curIntegralImageY++;
        }
      } else {
        curImageY = imageHeight - 1;
      }

      this->maxRow = curImageY;
      //this->minRow = MAX(0, this->maxRow-integralImageHeight+1);

      // If we're at the bottom of the image, compute the extra bottom padded rows
      if(numRowsToScroll > 0) {
        //paddedImageRow = padImageRow(image, curImageY, numBorderPixels);
        if(PadImageRow(image, curImageY, paddedRow) != RESULT_OK)
          return RESULT_FAIL;

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
      }

      return RESULT_OK;
    }

    Result ScrollingIntegralImage_u8_s32::FilterRow(const C_Acceleration &acceleration, const Rectangle<s16> &filter, const s32 imageRow, Array<s32> &output)
    {
      assert(this->IsValid());
      assert(output.IsValid());
      //assert(this->minRow >= 0);

      // TODO: support these
      assert(filter.left <= 0);
      assert(filter.top <= 0);
      assert(filter.right >= 0);
      assert(filter.bottom >= 0);

      const bool insufficientBorderPixels_left = (-filter.left+1) > this->numBorderPixels;
      const bool insufficientBorderPixels_right = (filter.right) > this->numBorderPixels;

      // These min and max coordinates are in the original image's coordinate frame
      const s32 minX = insufficientBorderPixels_left ? -(this->numBorderPixels + filter.left - 1) : 0;
      const s32 maxX = insufficientBorderPixels_right ? (this->get_size(1) - 1 - filter.right + this->numBorderPixels) : (this->get_imageWidth()-1);

      // Get the four pointers to the corners of the filter in the integral image.
      // The x offset is added at the end, because it might be invalid for x==0.
      // The -1 terms are because the rectangular sums should be inclusive.
      /*const s32 topOffset = imageRow + filter.top - 1 + this->numBorderPixels - this->rowOffset + this->numBorderPixels;
      const s32 bottomOffset = imageRow + filter.bottom + this->numBorderPixels - this->rowOffset + this->numBorderPixels;*/
      const s32 topOffset = imageRow - this->rowOffset + filter.top - 1 ;
      const s32 bottomOffset = imageRow - this->rowOffset + filter.bottom;
      const s32 leftOffset = filter.left - 1 + this->numBorderPixels;
      const s32 rightOffset = filter.right + this->numBorderPixels;

      const s32 * restrict integralImage_00 = this->Pointer(topOffset, 0) + leftOffset;
      const s32 * restrict integralImage_01 = this->Pointer(topOffset, 0) + rightOffset;
      const s32 * restrict integralImage_10 = this->Pointer(bottomOffset, 0) + leftOffset;
      const s32 * restrict integralImage_11 = this->Pointer(bottomOffset, 0) + rightOffset;

      s32 * restrict pOutput = output.Pointer(0,0);

      if(acceleration.type == C_ACCELERATION_NATURAL_CPP) {
        for(s32 x=0; x<minX; x++) {
          pOutput[x] = 0;
        }

        for(s32 x=minX; x<=maxX; x++) {
          pOutput[x] = integralImage_11[x] - integralImage_10[x] + integralImage_00[x] - integralImage_01[x];
        }

        for(s32 x=maxX+1; x<imageWidth; x++) {
          pOutput[x] = 0;
        }
      } else if(acceleration.type == C_ACCELERATION_NATURAL_C) {
        ScrollingIntegralImage_u8_s32_FilterRow_innerLoop(integralImage_00, integralImage_01, integralImage_10, integralImage_11, minX, maxX, this->imageWidth, pOutput);
      } else if(acceleration.type == C_ACCELERATION_SHAVE_EMULATION_C) {
        assert(output.get_size(1) >= (this->imageWidth+4));
        ScrollingIntegralImage_u8_s32_FilterRow_innerLoop_emulateShave(integralImage_00, integralImage_01, integralImage_10, integralImage_11, minX, maxX, this->imageWidth, pOutput);
      } else {
        assert(false);
      }

      return RESULT_OK;
    }

    // This might be used later, but not yet
#if 0
    Result ScrollingIntegralImage_u8_s32::FilterRow_c(const C_Acceleration &acceleration, const Rectangle<s16> &filter, const s32 imageRow, Array<s32> &output)
    {
      // TODO: verify that this conversion overhead is low

      const C_ScrollingIntegralImage_u8_s32 integralImage_c = this->get_cInterface();
      const C_Rectangle_s16 filter_c = get_C_Rectangle_s16(filter);
      C_Array_s32 output_c = get_C_Array_s32(output);

      const s32 result = ScrollingIntegralImage_u8_s32_FilterRow(&integralImage_c, &filter_c, imageRow, &output_c);

      return static_cast<Result>(result);
    }
#endif // #if 0

    // TODO: implement
    //s32 ScrollingIntegralImage_u8_s32::get_minRow(const s32 filterHalfHeight) const
    //{
    //  assert(filterHalfHeight >= 0);

    //  //const s32 shiftedMinRow = minRow + filterHalfHeight + this->rowOffset + 1;
    //  const s32 shiftedMinRow = minRow + filterHalfHeight - 1;

    //  return shiftedMinRow;
    //}

    s32 ScrollingIntegralImage_u8_s32::get_maxRow(const s32 filterHalfHeight) const
    {
      assert(filterHalfHeight >= 0);

      // TODO: make cleaner ?

      s32 shiftedMaxRow = maxRow - filterHalfHeight;
      const s32 bottomOffset = shiftedMaxRow - this->rowOffset + filterHalfHeight;

      if(bottomOffset < (this->size[0]-1)) {
        shiftedMaxRow += (this->size[0]-1) - bottomOffset;
      }

      //return maxRow - filterHalfHeight;

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

    Result ScrollingIntegralImage_u8_s32::PadImageRow_unsafe(const Array<u8> &image, const s32 whichRow, Array<u8> &paddedRow)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      assert(image.IsValid());
      assert(paddedRow.IsValid());
      assert(paddedRow.get_size(1) == (imageWidth+2*numBorderPixels));
      assert(paddedRow.get_size(0) == 1);
      assert(whichRow >= 0);
      assert(whichRow < imageHeight);

      const u8 * restrict pImage = image.Pointer(whichRow, 0);
      u8 * restrict pPaddedRow = paddedRow.Pointer(0, 0);

      const u8 firstPixel = pImage[0];
      const u8 lastPixel = pImage[imageWidth-1];

      //paddedImageRow = zeros([1,size(image,2)+2*extraBorderWidth], 'int32');
      s32 xPad = 0;

      // paddedImageRow(1:extraBorderWidth) = image(y,1);
      for(s32 x=0; x<numBorderPixels; x++) {
        pPaddedRow[xPad++] = firstPixel;
      }

      //paddedImageRow((1+extraBorderWidth):(extraBorderWidth+size(image,2))) = image(y,:);
      for(s32 xImage = 0; xImage<imageWidth; xImage++) {
        pPaddedRow[xPad++] = pImage[xImage];
      }

      //paddedImageRow((1+extraBorderWidth+size(image,2)):end) = image(y,end);
      for(s32 x=0; x<numBorderPixels; x++) {
        pPaddedRow[xPad++] = lastPixel;
      }

      return RESULT_OK;
    }

    Result ScrollingIntegralImage_u8_s32::PadImageRow(const Array<u8> &image, const s32 whichRow, Array<u8> &paddedRow)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
      const s32 imageWidth4 = imageWidth >> 2;

      assert(image.IsValid());
      assert(paddedRow.IsValid());
      assert(paddedRow.get_size(1) == (imageWidth+2*numBorderPixels));
      assert(paddedRow.get_size(0) == 1);
      assert(whichRow >= 0);
      assert(whichRow < imageHeight);

      const u32 * restrict image_u32rowPointer = reinterpret_cast<const u32*>(image.Pointer(whichRow, 0));
      const u8 * restrict image_u8rowPointer = image.Pointer(whichRow, 0);

      u8 * restrict paddedRow_u8rowPointer = paddedRow.Pointer(0, 0);

#ifdef BIG_ENDIAN_IMAGES
      const u8 firstPixel = (image_u32rowPointer[0] & 0xFF000000) >> 24;
      const u8 lastPixel = image_u32rowPointer[imageWidth4-1] & 0xFF;
#else

      const u8 firstPixel = image_u8rowPointer[0];
      const u8 lastPixel = image_u8rowPointer[imageWidth-1];
#endif

      s32 xPad = 0;

      xPad += numBorderPixels;

      //u32 * restrict paddedRow_u32rowPointerPxpad = reinterpret_cast<u32*>(paddedRow.Pointer(0, xPad));

      // First, load everything to the beginning of the buffer (SPARC doesn't have unaligned store)
      u32 * restrict paddedRow_u32rowPointerPxpad = reinterpret_cast<u32*>(paddedRow_u8rowPointer);
      for(s32 xImage = 0; xImage<imageWidth4; xImage++) {
#ifdef BIG_ENDIAN_IMAGES
        paddedRow_u32rowPointerPxpad[xImage] = SwapEndianU32(image_u32rowPointer[xImage]);
#else
        paddedRow_u32rowPointerPxpad[xImage] = image_u32rowPointer[xImage];
#endif
      }

      // Second, move the image data to the unaligned location
      memmove(paddedRow_u8rowPointer+xPad, paddedRow_u8rowPointer, imageWidth);

      // Last, set the beginning and end pixels
      xPad = 0;
      for(s32 x=0; x<numBorderPixels; x++) {
        paddedRow_u8rowPointer[xPad++] = firstPixel;
      }

      xPad += imageWidth;
      for(s32 x=0; x<numBorderPixels; x++) {
        paddedRow_u8rowPointer[xPad++] = lastPixel;
      }

      return RESULT_OK;
    }

    void ScrollingIntegralImage_u8_s32::ComputeIntegralImageRow(const u8* restrict paddedImage_currentRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth)
    {
      //integralImage_currentRow[0] = paddedImage_currentRow[0];

      //for(s32 x=1; x<integralImageWidth; x++) {
      //  integralImage_currentRow[x] = paddedImage_currentRow[x] + integralImage_currentRow[x-1];
      //}

      s32 horizontalSum = 0;
      for(s32 x=0; x<integralImageWidth; x++) {
        horizontalSum += paddedImage_currentRow[x];
        integralImage_currentRow[x] = horizontalSum;
      }
    }

    void ScrollingIntegralImage_u8_s32::ComputeIntegralImageRow(const u8 * restrict paddedImage_currentRow, const s32 * restrict integralImage_previousRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth)
    {
      //integralImage_currentRow[0] = paddedImage_currentRow[0] + integralImage_previousRow[0];

      //for(s32 x=1; x<integralImageWidth; x++) {
      //  integralImage_currentRow[x] = paddedImage_currentRow[x] + integralImage_currentRow[x-1] + integralImage_previousRow[x] - integralImage_previousRow[x-1];
      //}

      s32 horizontalSum = 0;
      for(s32 x=0; x<integralImageWidth; x++) {
        horizontalSum += paddedImage_currentRow[x];
        integralImage_currentRow[x] = horizontalSum + integralImage_previousRow[x];
      }
    }

    C_ScrollingIntegralImage_u8_s32 ScrollingIntegralImage_u8_s32::get_cInterface()
    {
      C_ScrollingIntegralImage_u8_s32 cVersion;

      cVersion.size0 = this->size[0];
      cVersion.size1 = this->size[1];
      cVersion.stride = this->stride;
      cVersion.useBoundaryFillPatterns = this->useBoundaryFillPatterns;

      cVersion.data = this->data;

      cVersion.imageWidth = this->imageWidth;
      cVersion.maxRow = this->maxRow;
      cVersion.rowOffset = this->rowOffset;
      cVersion.numBorderPixels = this->numBorderPixels;

      return cVersion;
    }
  } // namespace Embedded
} //namespace Anki