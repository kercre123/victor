/**
File: lucasKanade_KLT.cpp
Author: Peter Barnum
Created: 2014-09-30

A pyramidal KLT tracker, based off opencv.

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

#include "coretech/vision/robot/imageProcessing.h"
#include "coretech/common/robot/fixedLengthList.h"
#include "coretech/common/robot/matlabInterface.h"

using namespace Anki;
using namespace Anki::Embedded;

#define  CV_DESCALE(x,n)     (((x) + (1 << ((n)-1))) >> (n))
#define  CV_FLT_TO_FIX(x,n)  Round<s32>((x)*(1<<(n)))

static Result ComputeLKUpdate(
  const Array<u8> &prevImage,
  const Array<s8> &prevDerivX,
  const Array<s8> &prevDerivY,
  const Array<u8> &nextImage,
  const FixedLengthList<Point<f32> > &prevPoints,
  FixedLengthList<Point<f32> > &nextPoints,
  FixedLengthList<bool> &status,
  FixedLengthList<f32> &err,
  const s32 windowHeight,
  const s32 windowWidth,
  const s32 termination_maxCount,
  const f32 termination_epsilon,
  const f32 minEigThreshold,
  const bool usePreviousFlowAsInit,
  const s32 curPyramidLevel,
  const s32 maxPyramidLevel,
  MemoryStack scratch)
{
  const f32 FLT_SCALE = 1.f/(1 << 20);
  const s32 W_BITS = 14;

  const Point<f32> halfWin(static_cast<f32>(windowWidth-1) * 0.5f, static_cast<f32>(windowHeight-1) * 0.5f);

  const s32 imageHeight = prevImage.get_size(0);
  const s32 imageWidth = prevImage.get_size(1);

  //const Array<u8> &I = prevImage;
  //const Array<u8> &J = nextImage;
  //const Mat& derivI = *prevDeriv;

  const Point<f32> * restrict pPrevPts = prevPoints.Pointer(0);
  Point<f32> * restrict pNextPts = nextPoints.Pointer(0);
  bool * restrict pStatus = status.Pointer(0);
  f32 * restrict pErr = err.Pointer(0);

  // OpenCV uses s16 for derivatives, though the inputs here are s8
  Array<s16> IWinBuf(windowHeight, windowWidth, scratch);
  Array<s16> derivXIWinBuf(windowHeight, windowWidth, scratch);
  Array<s16> derivYIWinBuf(windowHeight, windowWidth, scratch);

  AnkiConditionalErrorAndReturnValue(AreValid(IWinBuf, derivXIWinBuf, derivYIWinBuf),
    RESULT_FAIL_OUT_OF_MEMORY, "ComputeLKUpdate", "Out of memory");

  const s32 numPoints = nextPoints.get_size();

  for( s32 ptidx = 0; ptidx < numPoints; ptidx++ )
  {
    Point<f32> prevPointF32 = pPrevPts[ptidx];
    prevPointF32 *= (f32)(1./(1 << curPyramidLevel));

    Point<f32> nextPointF32;
    if( curPyramidLevel == maxPyramidLevel ) {
      if(usePreviousFlowAsInit) {
        nextPointF32 = pNextPts[ptidx];
        nextPointF32 *= (f32)(1./(1 << curPyramidLevel));
      } else {
        nextPointF32 = prevPointF32;
      }
    } else {
      nextPointF32 = pNextPts[ptidx];
      nextPointF32 *= 2.f;
    }

    pNextPts[ptidx] = nextPointF32;

    Point<s32> prevPointS32, nextPointS32;
    prevPointF32 -= halfWin;
    prevPointS32.x = static_cast<s32>(floorf((prevPointF32.x)));
    prevPointS32.y = static_cast<s32>(floorf((prevPointF32.y)));

    // TODO: Are these set correctly?
    if( prevPointS32.x < 0 || prevPointS32.x >= (imageWidth-windowWidth) ||
      prevPointS32.y < 0 || prevPointS32.y >= (imageHeight-windowHeight) )
    {
      if( curPyramidLevel == 0 )
      {
        pStatus[ptidx] = false;
        pErr[ptidx] = 0;
      }
      continue;
    }

    f32 A11 = 0, A12 = 0, A22 = 0;
    {
      const f32 a = prevPointF32.x - prevPointS32.x;
      const f32 b = prevPointF32.y - prevPointS32.y;

      const s32 iw00 = Round<s32>((1.f - a)*(1.f - b)*(1 << W_BITS));
      const s32 iw01 = Round<s32>(a*(1.f - b)*(1 << W_BITS));
      const s32 iw10 = Round<s32>((1.f - a)*b*(1 << W_BITS));
      const s32 iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;

      //s32 dstep = (s32)(derivprevImage.step/derivprevImage.elemSize1());
      const s32 stepPrevDeriv = prevDerivX.get_stride();
      const s32 stepPrevImg = prevImage.get_stride();

      AnkiAssert(prevDerivX.get_stride() == prevDerivY.get_stride());

      // extract the patch from the first image, compute covariation matrix of derivatives
      for( s32 y = 0; y < windowHeight; y++ ) {
        //const u8* src = (const u8*)prevImage.data + (y + prevPointS32.y)*stepPrevImg + prevPointS32.x;
        //const s8* dsrc = (const s8*)derivprevImage.data + (y + prevPointS32.y)*dstep + prevPointS32.x*xxx;
        const u8 * restrict pPrevImg = prevImage.Pointer(y + prevPointS32.y, prevPointS32.x);
        const s8 * restrict pPrevImgDx = prevDerivX.Pointer(y + prevPointS32.y, prevPointS32.x);
        const s8 * restrict pPrevImgDy = prevDerivY.Pointer(y + prevPointS32.y, prevPointS32.x);

        //s16* pIWinBuf = (s8*)(IWinBuf.data + y*IWinBuf.step);
        //s16* dIptr = (s8*)(derivIWinBuf.data + y*derivIWinBuf.step);
        s16 * restrict pIWinBuf = IWinBuf.Pointer(y, 0);
        s16 * restrict pDerivXIWinBuf = derivXIWinBuf.Pointer(y, 0);
        s16 * restrict pDerivYIWinBuf = derivYIWinBuf.Pointer(y, 0);

        for(s32 x = 0; x < windowWidth; x++) {
          // Bilinear interpolate the value of the previous image
          const s32 ival  = CV_DESCALE(pPrevImg[x]*iw00 + pPrevImg[x+1]*iw01 + pPrevImg[x+stepPrevImg]*iw10 + pPrevImg[x+stepPrevImg+1]*iw11, W_BITS-5);

          // Bilinear interpolate the value of the x and y gradients of the previous image
          // The 32 multiplier is 2x for the change from -255:255, and 16x for the scaling for the Scharr (which could go up to 16*255).
          const s32 ixval = CV_DESCALE(32 * (pPrevImgDx[x]*iw00 + pPrevImgDx[x+1]*iw01 + pPrevImgDx[x+stepPrevDeriv]*iw10 + pPrevImgDx[x+stepPrevDeriv+1]*iw11), W_BITS);
          const s32 iyval = CV_DESCALE(32 * (pPrevImgDy[x]*iw00 + pPrevImgDy[x+1]*iw01 + pPrevImgDy[x+stepPrevDeriv]*iw10 + pPrevImgDy[x+stepPrevDeriv+1]*iw11), W_BITS);

          pIWinBuf[x] = (short)ival;
          pDerivXIWinBuf[x] = (short)ixval;
          pDerivYIWinBuf[y] = (short)iyval;

          A11 += (f32)(ixval*ixval);
          A12 += (f32)(ixval*iyval);
          A22 += (f32)(iyval*iyval);
        } // for(s32 x = 0; x < windowWidth; x++)
      } // for( s32 y = 0; y < windowHeight; y++ )
    } // f32 A11 = 0, A12 = 0, A22 = 0;

    A11 *= FLT_SCALE;
    A12 *= FLT_SCALE;
    A22 *= FLT_SCALE;

    f32 D = A11*A22 - A12*A12;
    const f32 minEig = (A22 + A11 - sqrtf((A11-A22)*(A11-A22) + 4.f*A12*A12))/(2*windowWidth*windowHeight);

    pErr[ptidx] = minEig;

    if( minEig < minEigThreshold || D < FLT_EPSILON )
    {
      if(curPyramidLevel == 0)
        pStatus[ptidx] = false;

      continue;
    }

    D = 1.f/D;

    nextPointF32 -= halfWin;
    Point<f32> prevDeltaF32;

    const s32 stepNextImg = nextImage.get_stride();

    s32 iteration;
    for(iteration = 0; iteration < termination_maxCount; iteration++ ) {
      nextPointS32.x = static_cast<s32>(floorf((nextPointF32.x)));
      nextPointS32.y = static_cast<s32>(floorf((nextPointF32.y)));

      // TODO: Are these set correctly?
      if( nextPointS32.x < 0 || nextPointS32.x >= (imageWidth-windowWidth) ||
        nextPointS32.y < 0 || nextPointS32.y >= (imageHeight-windowHeight) )
      {
        if(curPyramidLevel == 0)
          pStatus[ptidx] = false;

        break;
      }

      const f32 a = nextPointF32.x - nextPointS32.x;
      const f32 b = nextPointF32.y - nextPointS32.y;
      const s32 iw00 = Round<s32>((1.f - a)*(1.f - b)*(1 << W_BITS));
      const s32 iw01 = Round<s32>(a*(1.f - b)*(1 << W_BITS));
      const s32 iw10 = Round<s32>((1.f - a)*b*(1 << W_BITS));
      const s32 iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;
      f32 b1 = 0, b2 = 0;

      for( s32 y = 0; y < windowHeight; y++ ) {
        const u8 * restrict pNextImg = nextImage.Pointer(y + nextPointS32.y, nextPointS32.x);
        const s16 * restrict pIWinBuf = IWinBuf.Pointer(y, 0);
        const s16 * restrict pDerivXIWinBuf = derivXIWinBuf.Pointer(y, 0);
        const s16 * restrict pDerivYIWinBuf = derivYIWinBuf.Pointer(y, 0);

        for(s32 x = 0; x < windowWidth; x++) {
          s32 diff = CV_DESCALE(pNextImg[x]*iw00 + pNextImg[x+1]*iw01 + pNextImg[x+stepNextImg]*iw10 + pNextImg[x+stepNextImg+1]*iw11, W_BITS-5) - pIWinBuf[x];

          b1 += (f32)(diff*pDerivXIWinBuf[x]);
          b2 += (f32)(diff*pDerivYIWinBuf[y]);
        } // for(s32 x = 0; x < windowWidth; x++)
      } // for( s32 y = 0; y < windowHeight; y++ )

      b1 *= FLT_SCALE;
      b2 *= FLT_SCALE;

      const Point<f32> delta( (f32)((A12*b2 - A22*b1) * D), (f32)((A12*b1 - A11*b2) * D));

      nextPointF32 += delta;
      pNextPts[ptidx] = nextPointF32 + halfWin;

      const f32 deltaDot = delta.x*delta.x + delta.y*delta.y;
      if( deltaDot <= termination_epsilon )
        break;

      if( iteration > 0 &&
        ABS(delta.x + prevDeltaF32.x) < 0.01f &&
        ABS(delta.y + prevDeltaF32.y) < 0.01f )
      {
        Point<f32> halfDelta = delta;
        halfDelta *= 0.5f;
        pNextPts[ptidx] -= halfDelta;
        break;
      }
      prevDeltaF32 = delta;
    } // for(s32 iteration = 0; iteration < termination_maxCount; iteration++ )

    //printf("Point %d took %d iterations\n", ptidx, iteration);

    if( pStatus[ptidx] && curPyramidLevel == 0 ) {
      const Point<f32> nextPointShiftedF32 = pNextPts[ptidx] - halfWin;
      Point<s32> nextPointShiftedS32;

      nextPointShiftedS32.x = Round<s32>(nextPointShiftedF32.x);
      nextPointShiftedS32.y = Round<s32>(nextPointShiftedF32.y);

      if( nextPointShiftedS32.x < 0 || nextPointShiftedS32.x >= (imageWidth-windowWidth) ||
        nextPointShiftedS32.y < 0 || nextPointShiftedS32.y >= (imageHeight-windowHeight) )
      {
        pStatus[ptidx] = false;
        continue;
      }

      const f32 aa = nextPointShiftedF32.x - nextPointShiftedS32.x;
      const f32 bb = nextPointShiftedF32.y - nextPointShiftedS32.y;
      const s32 iw00 = Round<s32>((1.f - aa)*(1.f - bb)*(1 << W_BITS));
      const s32 iw01 = Round<s32>(aa*(1.f - bb)*(1 << W_BITS));
      const s32 iw10 = Round<s32>((1.f - aa)*bb*(1 << W_BITS));
      const s32 iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;
      f32 errval = 0.f;

      for( s32 y = 0; y < windowHeight; y++ ) {
        const u8 * restrict pNextImg = nextImage.Pointer(y + nextPointShiftedS32.y, nextPointShiftedS32.x);
        const s16 * restrict pIWinBuf = IWinBuf.Pointer(y, 0);

        for( s32 x = 0; x < windowWidth; x++ ) {
          s32 diff = CV_DESCALE(pNextImg[x]*iw00 + pNextImg[x+1]*iw01 + pNextImg[x+stepNextImg]*iw10 + pNextImg[x+stepNextImg+1]*iw11, W_BITS-5) - pIWinBuf[x];

          errval += ABS((f32)diff);
        } // for( s32 x = 0; x < windowWidth; x++ )
      } // for( s32 y = 0; y < windowHeight; y++ )

      pErr[ptidx] = errval * 1.f/(32*windowWidth*windowHeight);
    } // if( pStatus[ptidx] && curPyramidLevel == 0 )
  }

  return RESULT_OK;
} // ComputeLKUpdate()

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      Result CalcOpticalFlowPyrLK(
        const FixedLengthList<Array<u8> > &prevPyramid,
        const FixedLengthList<Array<u8> > &nextPyramid,
        const FixedLengthList<Point<f32> > &prevPoints,
        FixedLengthList<Point<f32> > &nextPoints,
        FixedLengthList<bool> &status,
        FixedLengthList<f32> &err,
        const s32 windowHeight,
        const s32 windowWidth,
        const s32 termination_maxCount,
        const f32 termination_epsilon,
        const f32 minEigThreshold,
        const bool usePreviousFlowAsInit,
        MemoryStack scratch)
      {
        const s32 numPyramidLevels = prevPyramid.get_size() - 1;
        const s32 numPrevPoints = prevPoints.get_size();

        //
        // Error check inputs
        //

        AnkiConditionalErrorAndReturnValue(AreValid(prevPyramid, nextPyramid, prevPoints, nextPoints, status, err),
          RESULT_FAIL_INVALID_OBJECT, "CalcOpticalFlowPyrLK", "Invalid inputs");

        AnkiConditionalErrorAndReturnValue(prevPyramid.get_size() == nextPyramid.get_size(),
          RESULT_FAIL_INVALID_PARAMETER, "CalcOpticalFlowPyrLK", "Invalid inputs");

        AnkiConditionalErrorAndReturnValue(
          prevPoints.get_size() <= nextPoints.get_maximumSize() &&
          nextPoints.get_maximumSize() == status.get_maximumSize() &&
          nextPoints.get_maximumSize() == err.get_maximumSize(),
          RESULT_FAIL_INVALID_PARAMETER, "CalcOpticalFlowPyrLK", "Invalid inputs");

        AnkiConditionalErrorAndReturnValue(
          windowHeight > 2 && windowWidth > 2 &&
          termination_maxCount > 0 && termination_maxCount <= 100 &&
          termination_epsilon >= 0 && termination_epsilon <= 10 &&
          minEigThreshold >= 0,
          RESULT_FAIL_INVALID_PARAMETER, "CalcOpticalFlowPyrLK", "Invalid inputs");

        {
          const s32 numImages = prevPyramid.get_size();
          const Array<u8> * restrict pPrevPyramid = prevPyramid.Pointer(0);
          const Array<u8> * restrict pNextPyramid = nextPyramid.Pointer(0);

          for(s32 i=0; i<numImages; i++) {
            AnkiConditionalErrorAndReturnValue(
              pPrevPyramid[i].IsValid() &&
              pPrevPyramid[i].get_size(0) > windowHeight && pPrevPyramid[i].get_size(1) > windowWidth &&
              pNextPyramid[i].IsValid() &&
              AreEqualSize(pPrevPyramid[i], pNextPyramid[i]),
              RESULT_FAIL_INVALID_OBJECT, "CalcOpticalFlowPyrLK", "Invalid inputs");
          }
        }

        nextPoints.set_size(numPrevPoints);
        status.set_size(numPrevPoints);
        err.set_size(numPrevPoints);

        status.Set(true);

        //const f32 epsilonSquared = termination_epsilon * termination_epsilon;

        if(!usePreviousFlowAsInit) {
          for(s32 i=0; i<numPrevPoints; i++) {
            nextPoints[i] = prevPoints[i];
          }
        }

        for(s32 curPyramidLevel = numPyramidLevels; curPyramidLevel >= 0; curPyramidLevel-- ) {
          PUSH_MEMORY_STACK(scratch);

          const Array<u8> &prevImage = prevPyramid[curPyramidLevel];
          //const Array<u8> &nextImage = nextPyramid[curPyramidLevel];

          const s32 imageHeight = prevImage.get_size(0);
          const s32 imageWidth = prevImage.get_size(1);

          Array<s8> dx(imageHeight, imageWidth, scratch);
          Array<s8> dy(imageHeight, imageWidth, scratch);

          AnkiConditionalErrorAndReturnValue(AreValid(dx, dy),
            RESULT_FAIL_OUT_OF_MEMORY, "CalcOpticalFlowPyrLK", "Out of memory");

          const Result gradientResult = ImageProcessing::FastGradient(prevImage, dx, dy, scratch);

          if(gradientResult != RESULT_OK)
            return gradientResult;

          const Result lkResult = ComputeLKUpdate(
            prevPyramid[curPyramidLevel],
            dx,
            dy,
            nextPyramid[curPyramidLevel],
            prevPoints,
            nextPoints,
            status,
            err,
            windowHeight,
            windowWidth,
            termination_maxCount,
            termination_epsilon,
            minEigThreshold,
            true,
            curPyramidLevel,
            nextPyramid.get_size() - 1,
            scratch);

          if(lkResult != RESULT_OK)
            return lkResult;
        } // for(s32 curPyramidLevel = numPyramidLevels; curPyramidLevel >= 0; curPyramidLevel-- )

        return RESULT_OK;
      } // CalcOpticalFlowPyrLK()
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
