/**
File: integralImage_declarations.h
Author: Peter Barnum
Created: 2014-05-09

A ScrollingIntegralImage is a class that hold a small part of an integral image in memory. The integral image is used to box filter.

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    class IntegralImage_u8_s32 : public Array<s32>
    {
    public:
      // Simple, memory-inefficient version of an integral image
      // The top row and left column are all zeros
      // The integral image has one more row and column than the original image

      IntegralImage_u8_s32();

      IntegralImage_u8_s32(const Array<u8> &image, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false));
    };

    class IntegralImage_u8_u16 : public Array<u16>
    {
    public:
      // Simple, memory-inefficient version of an integral image
      // The top row and left column are all zeros
      // The integral image has one more row and column than the original image

      IntegralImage_u8_u16();

      IntegralImage_u8_u16(const Array<u8> &image, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false));
    };

    // #pragma mark --- ScrollingIntegralImage Class Definition ---

    // A ScrollingIntegralImage computes and integral image from and input image. To save memory,
    // the entire Integral Image isn't computed at once. Instead, the ScrollingIntegralImage
    // computes and stores a small window, plus optional border padding.
    //
    // For example, for an input image of size MxN, a ScrollingIntegralImage could be of size OxN
    // (where O < N) The suffix is _InputType_AccumulatorType
    class ScrollingIntegralImage_u8_s32 : public Array<s32>
    {
    public:
      ScrollingIntegralImage_u8_s32();

      // Allocate this ScrollingIntegralImage from "MemoryStack memory".
      //
      // "imageWidth" is the size of the original image, not including the extra numBorderPixels.
      //
      // "bufferHeight" is the height of this ScrollingIntegralImage, not the original image
      //
      // This ScrollingIntegralImage will not be usable until it ScrollDown() is called, and at
      // least part of the integral image is valid.
      //
      // To allow outputs that don't have zero borders, this ScrollingIntegralImage can be created
      // with "numBorderPixels" extra pixels. For example, using a 5x5 filter would require an extra
      // 3 border pixels, because floor(5/2)+1 = 3. When numBorderPixels>0, then the outermost pixel
      // will be replicated to estimate the unknown values.
      ScrollingIntegralImage_u8_s32(const s32 bufferHeight, const s32 imageWidth, const s32 numBorderPixels, MemoryStack &memory, const Flags::Buffer flags=Flags::Buffer(true,false,false));

      // Using the data from "image", scroll this integral image down. For example, if the integral
      // image is windowed between rows 0 to 100, then scrolling down by 10 lines would make it
      // windowed between 10 to 110.
      Result ScrollDown(const Array<u8> &image, s32 numRowsToScroll, MemoryStack scratch);

      // Compute a box filter on one row of the image. The Rectangle filter defines offsets from the
      // center. For example, the Rectangle with (top,bottom,left,right) = (-2,2,-3,3) will sum with
      // a rectangular area that is 5 high and 7 wide.
      //
      // The variable "imageRow" refers to the row on
      // the original "Array<u8> image", not the offset from the top of this ScrollingIntegralImage.
      //
      // Note that the filter top and left coordinates are inclusive, and do not have to be offset
      // by -1 as with normal integral image filtering. So the filter between (3,3) and (5,5) will
      // sum 9 different pixels, not 4 different pixels.
      //
      // If the filter is larger than this ScrollingIntegralImage's numBorderPixels, then the
      // borders will be zeros.
      Result FilterRow(const Rectangle<s16> &filter, const s32 imageRow, Array<s32> &output) const;

      // Same as the above, but the final output = (filtered * outputMultiply) >> outputShift
      // And output other than s32 is supported
      // Useful for normalization or scaling
      template<typename OutType> Result FilterRow(const Rectangle<s16> &filter, const s32 imageRow, const s32 outputMultiply, const s32 outputRightShift, Array<OutType> &output) const;

      // Get the current min and max rows that can be filtered with FilterRow(), for a give filter
      // that extends filterHalfHeight above or below its center.
      //
      // Note that for a given filterHalfHeight, the minRow can be greater than the maxRow. This
      // will happen if the filter size is too large for this ScrollingIntegralImage, or due to
      // assymetric filters.
      //s32 get_minRow(const s32 filterHalfHeight) const;
      s32 get_maxRow(const s32 filterHalfHeight) const;

      // Return the width of the original image (this integral image is imageWidth + 2*numBorderPixels wide).
      s32 get_imageWidth() const;

      s32 get_rowOffset() const;

      s32 get_numBorderPixels() const;

    protected:
      s32 imageWidth; //< width of the original image
      //s32 minRow, maxRow; //< Min and max rows that can be filtered with a height==1 filter
      s32 maxRow;
      s32 rowOffset; //< Row 0 of this integral image corresponds to which row of the original image (can be negative)
      s32 numBorderPixels;

      // Compute the first row of an integral image
      static void ComputeIntegralImageRow(const u8 * restrict paddedImage_currentRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth);

      // Compute the nth row of an integral image
      static void ComputeIntegralImageRow(const u8 * restrict paddedImage_currentRow, const s32 * restrict integralImage_previousRow, s32 * restrict integralImage_currentRow, const s32 integralImageWidth);

      // Virtually zero-pads to the left and right of an image row
      Result PadImageRow(const Array<u8> &image, const s32 whichRow, Array<u8> &paddedRow);

      template<typename OutType> static void FilterRow_innerLoop(
        const s32 minX,
        const s32 maxX,
        const s32 outputMultiply,
        const s32 outputRightShift,
        const s32 * restrict pIntegralImage_00,
        const s32 * restrict pIntegralImage_01,
        const s32 * restrict pIntegralImage_10,
        const s32 * restrict pIntegralImage_11,
        OutType * restrict pOutput);
    };

    template<> void ScrollingIntegralImage_u8_s32::FilterRow_innerLoop(
      const s32 minX,
      const s32 maxX,
      const s32 outputMultiply,
      const s32 outputRightShift,
      const s32 * restrict pIntegralImage_00,
      const s32 * restrict pIntegralImage_01,
      const s32 * restrict pIntegralImage_10,
      const s32 * restrict pIntegralImage_11,
      u8 * restrict pOutput);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_INTEGRAL_IMAGE_DECLARATIONS_H_
