/**
File: imageProcessing_declarations.h
Author: Peter Barnum
Created: 2013

Miscellaneous image processing utilities

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
#define BINOMIAL_FILTER_KERNEL_SIZE 5

      //
      // Simple filtering
      //

      // Compute the [-1, 0, 1] gradient
      template<typename InType, typename IntermediateType, typename OutType> Result ComputeXGradient(const Array<InType> &in, Array<OutType> &out);
      template<typename InType, typename IntermediateType, typename OutType> Result ComputeYGradient(const Array<InType> &in, Array<OutType> &out);

      // Note: Output gradient is divided by two, so the output is between [-127,127]
      Result FastGradient(const Array<u8> &in, Array<s8> &dx, Array<s8> &dy, MemoryStack scratch);

      // Filter with a horizontal and vertical [1, 4, 6, 4, 1] filter
      // Handles edges by replicating the border pixel
      template<typename InType, typename IntermediateType, typename OutType> Result BinomialFilter(const Array<InType> &image, Array<OutType> &imageFiltered, MemoryStack scratch);
      template<> Result BinomialFilter<u8,u8,u8>(const Array<u8> &image, Array<u8> &imageFiltered, MemoryStack scratch);

      // A 1 dimensional, fixed point Gaussian kernel
      template<typename Type> FixedPointArray<Type> Get1dGaussianKernel(const s32 sigma, const s32 numSigmaFractionalBits, const s32 numStandardDeviations, MemoryStack &scratch);

      template<typename InType, typename IntermediateType, typename OutType> Result Correlate1d(const FixedPointArray<InType> &in1, const FixedPointArray<InType> &in2, FixedPointArray<OutType> &out);

      //// NOTE: uses a 32-bit accumulator, so be careful of overflows
      //Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &image, const FixedPointArray<s32> &filter, FixedPointArray<s32> &out);
      //Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s16> &image, const FixedPointArray<s32> &filter, FixedPointArray<s32> &out);

      template<typename InType, typename IntermediateType, typename OutType> Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<InType> &image, const FixedPointArray<InType> &filter, FixedPointArray<OutType> &out, MemoryStack scratch);

      // Populate a full integral image created from the input image.
      template<typename InType, typename OutType>
      Result CreateIntegralImage(const Array<InType> &image, Array<OutType> integralImage);

      // Divide each pixel by the box-filter average of its neighborhood
      // This is not memory efficient: it internally creates a full f32 integral
      // image the same size as the input image, and thus requires enough
      // scratch memory to store that integral image.
      Result BoxFilterNormalize(const Array<u8> &image, const s32 boxSize, const u8 padValue,
        Array<u8> &imageNorm, MemoryStack scratch);

      // Fast box filter
      // Input Arrays "image" and "filtered" cannot be the same
      // Makes a border of zeros when the box filter goes over the image border
      template<typename InType, typename IntermediateType, typename OutType> Result BoxFilter(const Array<InType> &image, const s32 boxHeight, const s32 boxWidth, Array<OutType> &filtered, MemoryStack scratch);
      template<> Result BoxFilter<u8,u16,u16>(const Array<u8> &image, const s32 boxHeight, const s32 boxWidth, Array<u16> &filtered, MemoryStack scratch);

      //
      // Image resizing
      //

      // Linearly interpolate a resized image. Not as efficient as DownsampleByTwo or DownsampleByPowerOfTwo
      // Should give the same results as Matlab's imresize, with 'AntiAliasing' == false
      template<typename InType, typename OutType> Result Resize(const Array<InType> &in, Array<OutType> &out);

      Result DownsampleBilinear(const Array<u8> &in, Array<u8> &out, MemoryStack scratch);

      template<int upsamplePower> Result UpsampleByPowerOfTwoBilinear(const Array<u8> &in, Array<u8> &out, MemoryStack scratch);

      // Just calls the template version for upsamplePower < 0 && upsamplePower < 6
      //Result UpsampleByPowerOfTwoBilinear(const Array<u8> &in, const s32 upsamplePower, Array<u8> &out, MemoryStack scratch);

      template<typename InType, typename IntermediateType, typename OutType> Result DownsampleByTwo(const Array<InType> &in, Array<OutType> &out);

      // Downsample the image by 2^downsamplePower.
      // For example, if downsampleFactor==2, then a 640x480 image will be converted to a 160x120 image
      // This function is safe if Array "in" is in DDR (I think)
      // Same output as Matlab's imresize, with 'Method'=='box'
      template<typename InType, typename IntermediateType, typename OutType> Result DownsampleByPowerOfTwo(const Array<InType> &in, const s32 downsamplePower, Array<OutType> &out, MemoryStack scratch);

      // Build a 2x downsampled pyramid from an image
      template<typename InType, typename IntermediateType, typename OutType> FixedLengthList<Array<OutType> > BuildPyramid(
        const Array<InType> &image, //< WARNING: the memory for "image" is used by the first level of the pyramid.
        const s32 numPyramidLevels, //< The number of levels in the pyramid is numPyramidLevels + 1, so can be 0 for a single image. The image size must be evenly divisible by "2^numPyramidLevels".
        MemoryStack &memory);  //< Memory for the output will be allocated by this function

      //
      // Color processing
      //

      Result YUVToGrayscale(const Array<u16> &yuvImage, Array<u8> &grayscaleImage);

      //
      // 2D-to-2D transformations
      //

      // 3x3 window, fixed-point, approximate Euclidian distance transform. Based on Borgefors "Distance transformations in digital images" 1986
      // Any pixel with grayvalue less than backgroundThreshold is treated as background
      Result DistanceTransform(const Array<u8> image, const u8 backgroundThreshold, FixedPointArray<s16> &distance);

      //
      // Morphology
      //

      // Find all searchHeightxsearchWidth local maxima (only 3x3 currently supported)
      // Optionally return the values at those points
      template<typename Type> Result LocalMaxima(const Array<Type> &in, const s32 searchHeight, const s32 searchWidth, FixedLengthList<Point<s16> > &points, FixedLengthList<Type> *values=NULL);
    } // namespace ImageProcessing
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_DECLARATIONS_H_
