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

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d_declarations.h"

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

      // Filter with a horizontal and vertical [1, 4, 6, 4, 1] filter
      // Handles edges by replicating the border pixel
      template<typename InType, typename IntermediateType, typename OutType> Result BinomialFilter(const Array<InType> &image, Array<OutType> &imageFiltered, MemoryStack scratch);

      FixedPointArray<s32> Get1dGaussianKernel(const s32 sigma, const s32 numSigmaFractionalBits, const s32 numStandardDeviations, MemoryStack &scratch);

      // NOTE: uses a 32-bit accumulator, so be careful of overflows
      Result Correlate1d(const FixedPointArray<s32> &in1, const FixedPointArray<s32> &in2, FixedPointArray<s32> &out);

      // NOTE: uses a 32-bit accumulator, so be careful of overflows
      Result Correlate1dCircularAndSameSizeOutput(const FixedPointArray<s32> &image, const FixedPointArray<s32> &filter, FixedPointArray<s32> &out);

      //
      // Image resizing
      //

      // Linearly interpolate a resized image. Not as efficient as DownsampleByTwo or DownsampleByPowerOfTwo
      // Should give the same results as Matlab's imresize, with 'AntiAliasing' == false
      template<typename InType, typename OutType> Result Resize(const Array<InType> &in, Array<OutType> &out);

      template<typename InType, typename IntermediateType, typename OutType> Result DownsampleByTwo(const Array<InType> &in, Array<OutType> &out);

      // Downsample the image by 2^downsamplePower.
      // For example, if downsampleFactor==2, then a 640x480 image will be converted to a 160x120 image
      // This function is safe if Array "in" is in DDR (I think)
      // Same output as Matlab's imresize, with 'Method'=='box'
      template<typename InType, typename IntermediateType, typename OutType> Result DownsampleByPowerOfTwo(const Array<InType> &in, const s32 downsamplePower, Array<OutType> &out, MemoryStack scratch);

      //
      // Color processing
      //

      Result YUVToGrayscale(const Array<u16> &yuvImage, Array<u8> &grayscaleImage);
    } // namespace ImageProcessing
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_DECLARATIONS_H_
