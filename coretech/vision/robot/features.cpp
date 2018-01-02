/**
File: features.cpp
Author: Peter Barnum
Created: 2014-09-30

Good features to track, based off opencv

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "coretech/vision/robot/classifier.h"

#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/hostIntrinsics_m4.h"
#include "coretech/common/robot/matrix.h"

#include "coretech/vision/robot/imageProcessing.h"

//#if ANKICORETECH_EMBEDDED_USE_OPENCV
//#include "opencv2/objdetect/cascadedetect.hpp"
//#endif

using namespace Anki;
using namespace Anki::Embedded;

enum { MINEIGENVAL=0, HARRIS=1, EIGENVALSVECS=2 };

static Result calcHarris(
  const Array<f32> &cov_dxdx,
  const Array<f32> &cov_dxdy,
  const Array<f32> &cov_dydy,
  Array<f32> &dst,
  const f32 k)
{
  const s32 imageHeight = cov_dxdx.get_size(0);
  const s32 imageWidth = cov_dxdx.get_size(1);

  AnkiAssert(AreEqualSize(cov_dxdx, cov_dxdy, cov_dydy, dst));

  for(s32 y = 0; y < imageHeight; y++) {
    const f32 * restrict pCov_dxdx = cov_dxdx.Pointer(y,0);
    const f32 * restrict pCov_dxdy = cov_dxdy.Pointer(y,0);
    const f32 * restrict pCov_dydy = cov_dydy.Pointer(y,0);
    f32 * restrict pDst = dst.Pointer(y,0);

    for(s32 x=0; x<imageWidth; x++) {
      const f32 a = pCov_dxdx[x];
      const f32 b = pCov_dxdy[x];
      const f32 c = pCov_dydy[x];
      pDst[x] = a*c - b*b - k*(a + c)*(a + c);
    }
  }

  return RESULT_OK;
}

static Result cornerEigenValsVecs(
  const Array<u8> &src,
  Array<f32> &eigenv,
  const s32 block_size,
  const s32 op_type,
  const f32 k,
  MemoryStack scratch)
{
  AnkiConditionalErrorAndReturnValue(AreEqualSize(src, eigenv),
    RESULT_FAIL, "cornerEigenValsVecs", "Input and output must be the same size");

  const s32 imageHeight = src.get_size(0);
  const s32 imageWidth = src.get_size(1);

  Array<s8> dx(imageHeight, imageWidth, scratch);
  Array<s8> dy(imageHeight, imageWidth, scratch);

  AnkiConditionalErrorAndReturnValue(AreValid(dx, dy),
    RESULT_FAIL_OUT_OF_MEMORY, "cornerEigenValsVecs", "Out of memory");

  const Result gradientResult = ImageProcessing::FastGradient(src, dx, dy, scratch);

  if(gradientResult != RESULT_OK)
    return gradientResult;

  Array<f32> cov_dxdx(imageHeight, imageWidth, scratch);
  Array<f32> cov_dxdy(imageHeight, imageWidth, scratch);
  Array<f32> cov_dydy(imageHeight, imageWidth, scratch);

  Array<f32> blurredCov(imageHeight, imageWidth, scratch);

  AnkiConditionalErrorAndReturnValue(AreValid(cov_dxdx, cov_dxdy, cov_dydy, blurredCov),
    RESULT_FAIL_OUT_OF_MEMORY, "cornerEigenValsVecs", "Out of memory");

  for(s32 y = 0; y < imageHeight; y++) {
    const s8 * restrict pDx = dx.Pointer(y,0);
    const s8 * restrict pDy = dy.Pointer(y,0);

    f32 * restrict pCov_dxdx = cov_dxdx.Pointer(y,0);
    f32 * restrict pCov_dxdy = cov_dxdy.Pointer(y,0);
    f32 * restrict pCov_dydy = cov_dydy.Pointer(y,0);

    for(s32 x = 0; x < imageWidth; x++) {
      const f32 dx = pDx[x];
      const f32 dy = pDy[x];

      pCov_dxdx[x] = dx*dx;
      pCov_dxdy[x] = dx*dy;
      pCov_dydy[x] = dy*dy;
    }
  }

  const Result filterResult1 = ImageProcessing::BoxFilter<f32,f32,f32>(cov_dxdx, block_size, block_size, blurredCov, scratch);
  cov_dxdx.Set(blurredCov);

  const Result filterResult2 = ImageProcessing::BoxFilter<f32,f32,f32>(cov_dxdy, block_size, block_size, blurredCov, scratch);
  cov_dxdy.Set(blurredCov);

  const Result filterResult3 = ImageProcessing::BoxFilter<f32,f32,f32>(cov_dydy, block_size, block_size, blurredCov, scratch);
  cov_dydy.Set(blurredCov);

  if(filterResult1 != RESULT_OK || filterResult2 != RESULT_OK || filterResult3 != RESULT_OK) {
    return RESULT_FAIL;
  }

  if( op_type == MINEIGENVAL ) {
    AnkiAssert(false); // TODO: implement
  } else if( op_type == HARRIS ) {
    return calcHarris(cov_dxdx, cov_dxdy, cov_dydy, eigenv, k);
  } else if( op_type == EIGENVALSVECS ) {
    AnkiAssert(false); // TODO: implement
  }

  return RESULT_FAIL;
}

namespace Anki
{
  namespace Embedded
  {
    namespace Features
    {
      Result CornerHarris(
        const Array<u8> &src,
        Array<f32> &dst,
        const s32 blockSize,
        const f32 k,
        MemoryStack scratch)
      {
        return cornerEigenValsVecs(
          src, dst,
          blockSize, HARRIS, k,
          scratch);
      } // CornerHarris()

      Result GoodFeaturesToTrack(
        const Array<u8> &image,
        FixedLengthList<Point<s16> > &corners,
        const s32 maxCorners,
        const f32 qualityLevel,
        const s32 blockSize,
        const bool useHarrisDetector,
        const f32 harrisK,
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        // TODO: support more
        AnkiConditionalErrorAndReturnValue(useHarrisDetector,
          RESULT_FAIL_INVALID_PARAMETER, "GoodFeaturesToTrack", "Only Harris suuported");

        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        Array<f32> harrisImage(imageHeight, imageWidth, slowScratch);

        AnkiConditionalErrorAndReturnValue(harrisImage.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "GoodFeaturesToTrack", "Out of memory");

        // Compute the harris image
        const Result harrisResult = CornerHarris(image, harrisImage, blockSize, harrisK, slowScratch);

        AnkiConditionalErrorAndReturnValue(harrisResult == RESULT_OK,
          harrisResult, "GoodFeaturesToTrack", "Error");

        FixedLengthList<f32> values(corners.get_maximumSize(), fastScratch);

        AnkiConditionalErrorAndReturnValue(values.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "GoodFeaturesToTrack", "Out of memory");

        // Find the 3x3 local maxima
        const Result maximaResult = ImageProcessing::LocalMaxima<f32>(harrisImage, 3, 3, corners, &values);

        AnkiConditionalErrorAndReturnValue(maximaResult == RESULT_OK,
          harrisResult, "GoodFeaturesToTrack", "Error");

        // Remove values that are poor quality

        const s32 numValues = values.get_size();
        s32 numValuesNonzero = numValues;

        //const f32 * restrict pvalues = values.Pointer(0);

        f32 maxValue = FLT_MIN;
        for(s32 i=0; i<numValues; i++) {
          maxValue = MAX(maxValue, values[i]);
        }

        const f32 goodThreshold = maxValue * qualityLevel;

        for(s32 i=0; i<numValues; i++) {
          if(values[i] < goodThreshold) {
            values[i] = 0;
            numValuesNonzero--;
          }
        }

        //
        // Sort the values of the maxima, and select the top maxCorners
        //

        AnkiAssert(numValuesNonzero >= 0);

        if(numValuesNonzero == 0) {
          corners.set_size(0);

          return RESULT_OK;
        }

        Array<f32> sortedValues(1, values.get_size(), fastScratch);

        AnkiConditionalErrorAndReturnValue(sortedValues.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "GoodFeaturesToTrack", "Out of memory");

        memcpy(sortedValues.Pointer(0,0), values.Pointer(0), sizeof(f32)*values.get_size());

        const Result sortResult = Matrix::QuickSort<f32>(sortedValues, 1, false);

        AnkiConditionalErrorAndReturnValue(sortResult == RESULT_OK,
          harrisResult, "GoodFeaturesToTrack", "Error");

        const f32 valueThreshold = MAX(1e-5f, sortedValues[0][MIN(maxCorners-1, numValues-1)]);

        const f32 * restrict pValues = values.Pointer(0);
        Point<s16> * restrict pCorners = corners.Pointer(0);
        s32 goodIndex = 0;

        // NOTE: If there are lot of items with the same value, this may not strictly pick the best values. It may also select too many on the boundary.
        for(s32 i=0; i<numValues; i++) {
          if(goodIndex == maxCorners) {
            break;
          }

          if(pValues[i] >= valueThreshold) {
            pCorners[goodIndex] = pCorners[i];
            goodIndex++;
          }
        }

        corners.set_size(goodIndex);

        return RESULT_OK;
      } // GoodFeaturesToTrack()
    }
  } // namespace Embedded
} // namespace Anki
