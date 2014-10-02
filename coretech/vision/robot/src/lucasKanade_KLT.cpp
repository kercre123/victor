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

#include "anki/vision/robot/imageProcessing.h"

using namespace Anki;
using namespace Anki::Embedded;

#define  CV_DESCALE(x,n)     (((x) + (1 << ((n)-1))) >> (n))
#define  CV_FLT_TO_FIX(x,n)  cvRound((x)*(1<<(n)))

//prevPyr[level * 1],
//  derivI,
//  nextPyr[level * 1],
//  prevPts, nextPts,
//  status, err,
//  winSize, termination_maxCount, epsilonSquared, level, maxPyramidLevel,
//  flags, (f32)minEigThreshold));

//Result CalcOpticalFlowPyrLK(
//      const FixedLengthList<Array<u8> > &prevPyramid,
//      const FixedLengthList<Array<u8> > &nextPyramid,
//      const FixedLengthList<Point<f32> > &prevPts,
//      FixedLengthList<Point<f32> > &nextPts,
//      FixedLengthList<bool> &status,
//      FixedLengthList<f32> &err,
//      const s32 windowHeight,
//      const s32 windowWidth,
//      const s32 termination_maxCount,
//      const f32 termination_epsilon,
//      const f32 minEigThreshold,
//      MemoryStack scratch)

//prevImg = &_prevImg;
//    prevDeriv = &_prevDeriv;
//    nextImg = &_nextImg;
//    prevPts = _prevPts;
//    nextPts = _nextPts;
//    status = _status;
//    err = _err;
//    winSize = _winSize;
//    criteria = _criteria;
//    level = _level;
//    maxPyramidLevel = _maxPyramidLevel;
//    flags = _flags;
//    minEigThreshold = _minEigThreshold;

Result ComputeLKUpdate(
  const Array<u8> &prevImg2,
  const Array<s8> &prevDerivX2,
  const Array<s8> &prevDerivY2,
  const Array<u8> &nextImg2,
  const FixedLengthList<Point<f32> > &prevPts2,
  FixedLengthList<Point<f32> > &nextPts2,
  FixedLengthList<bool> &status2,
  FixedLengthList<f32> &err2,
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
  const Point<f32> halfWin(static_cast<f32>(windowWidth-1) * 0.5f, static_cast<f32>(windowHeight-1) * 0.5f);

  const s32 imageHeight = prevImg2.get_size(0);
  const s32 imageWidth = prevImg2.get_size(1);

  //const Array<u8> &I = prevImg;
  //const Array<u8> &J = nextImg;
  //const Mat& derivI = *prevDeriv;

  const Point<f32> * restrict pPrevPts = prevPts2.Pointer(0);
  Point<f32> * restrict pNextPts = nextPts2.Pointer(0);
  bool * restrict pStatus = status2.Pointer(0);
  f32 * restrict pErr = err2.Pointer(0);

  // OpenCV uses s16 for derivatives, though the inputs here are s8
  Array<s16> IWinBuf(windowHeight, windowWidth, scratch);
  Array<s16> derivXIWinBuf(windowHeight, windowWidth, scratch);
  Array<s16> derivYIWinBuf(windowHeight, windowWidth, scratch);

  const s32 numPoints = nextPts2.get_size();

  for( s32 ptidx = 0; ptidx < numPoints; ptidx++ )
  {
    Point<f32> prevPt = pPrevPts[ptidx];
    prevPt *= (f32)(1./(1 << curPyramidLevel));

    Point<f32> nextPt;
    if( curPyramidLevel == maxPyramidLevel ) {
      if(usePreviousFlowAsInit) {
        nextPt = pNextPts[ptidx];
        nextPt *= (f32)(1./(1 << curPyramidLevel));
      } else {
        nextPt = prevPt;
      }
    } else {
      nextPt = pNextPts[ptidx];
      nextPt *= 2.f;
    }

    pNextPts[ptidx] = nextPt;

    Point<s32> iprevPt, inextPt;
    prevPt -= halfWin;
    iprevPt.x = cvFloor(prevPt.x);
    iprevPt.y = cvFloor(prevPt.y);

    // TODO: set these bounds correctly
    if( iprevPt.x < -windowWidth || iprevPt.x >= imageWidth ||
      iprevPt.y < -windowHeight || iprevPt.y >= imageHeight )
    {
      if( curPyramidLevel == 0 )
      {
        pStatus[ptidx] = false;
        pErr[ptidx] = 0;
      }
      continue;
    }

    f32 a = prevPt.x - iprevPt.x;
    f32 b = prevPt.y - iprevPt.y;
    const s32 W_BITS = 14, W_BITS1 = 14;
    const f32 FLT_SCALE = 1.f/(1 << 20);
    s32 iw00 = cvRound((1.f - a)*(1.f - b)*(1 << W_BITS));
    s32 iw01 = cvRound(a*(1.f - b)*(1 << W_BITS));
    s32 iw10 = cvRound((1.f - a)*b*(1 << W_BITS));
    s32 iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;

    //s32 dstep = (s32)(derivprevImg.step/derivprevImg.elemSize1());
    const s32 stepPrevDeriv = prevDerivX2.get_stride();
    const s32 stepPrevImg = prevImg2.get_stride();
    const s32 stepNextImg = nextImg2.get_stride();

    AnkiAssert(prevDerivX2.get_stride() == prevDerivY2.get_stride());

    f32 A11 = 0, A12 = 0, A22 = 0;

    // extract the patch from the first image, compute covariation matrix of derivatives
    for( s32 y = 0; y < windowHeight; y++ ) {
      //const u8* src = (const u8*)prevImg.data + (y + iprevPt.y)*stepPrevImg + iprevPt.x;
      //const s8* dsrc = (const s8*)derivprevImg.data + (y + iprevPt.y)*dstep + iprevPt.x*xxx;
      const u8 * restrict pPrevImg = prevImg2.Pointer(y + iprevPt.y, iprevPt.x);
      const s8 * restrict pPrevImgDx = prevDerivX2.Pointer(y + iprevPt.y, iprevPt.x);
      const s8 * restrict pPrevImgDy = prevDerivY2.Pointer(y + iprevPt.y, iprevPt.x);

      //s16* pIWinBuf = (s8*)(IWinBuf.data + y*IWinBuf.step);
      //s16* dIptr = (s8*)(derivIWinBuf.data + y*derivIWinBuf.step);
      s16 * restrict pIWinBuf = IWinBuf.Pointer(y, 0);
      s16 * restrict pDerivXIWinBuf = derivXIWinBuf.Pointer(y, 0);
      s16 * restrict pDerivYIWinBuf = derivYIWinBuf.Pointer(y, 0);

      for(s32 x = 0; x < windowWidth; x++) {
        // Bilinear interpolate the value of the previous image
        const s32 ival  = CV_DESCALE(pPrevImg[x]*iw00 + pPrevImg[x+1]*iw01 + pPrevImg[x+stepPrevImg]*iw10 + pPrevImg[x+stepPrevImg+1]*iw11, W_BITS1-5);

        // Bilinear interpolate the value of the x and y gradients of the previous image
        // TODO: since I'm using the half-gradient s8, do I have to scale these differently?
        const s32 ixval = CV_DESCALE(pPrevImgDx[0]*iw00 + pPrevImgDx[1]*iw01 + pPrevImgDx[stepPrevDeriv]*iw10 + pPrevImgDx[stepPrevDeriv+1]*iw11, W_BITS1);
        const s32 iyval = CV_DESCALE(pPrevImgDy[0]*iw00 + pPrevImgDy[1]*iw01 + pPrevImgDy[stepPrevDeriv]*iw10 + pPrevImgDy[stepPrevDeriv+1]*iw11, W_BITS1);

        pIWinBuf[x] = (short)ival;
        dIptr[0] = (short)ixval;
        dIptr[1] = (short)iyval;

        A11 += (f32)(ixval*ixval);
        A12 += (f32)(ixval*iyval);
        A22 += (f32)(iyval*iyval);
      } // for(s32 x = 0; x < windowWidth; x++)
    } // for( s32 y = 0; y < windowHeight; y++ )

    A11 *= FLT_SCALE;
    A12 *= FLT_SCALE;
    A22 *= FLT_SCALE;

    f32 D = A11*A22 - A12*A12;
    f32 minEig = (A22 + A11 - std::sqrt((A11-A22)*(A11-A22) +
      4.f*A12*A12))/(2*windowWidth*windowHeight);

    pErr[ptidx] = (f32)minEig;

    if( minEig < minEigThreshold || D < FLT_EPSILON )
    {
      if(curPyramidLevel == 0)
        pStatus[ptidx] = false;

      continue;
    }

    D = 1.f/D;

    nextPt -= halfWin;
    Point<f32> prevDelta;

    for( j = 0; j < criteria.maxCount; j++ )
    {
      inextPt.x = cvFloor(nextPt.x);
      inextPt.y = cvFloor(nextPt.y);

      if( inextPt.x < -windowWidth || inextPt.x >= J.cols ||
        inextPt.y < -windowHeight || inextPt.y >= J.rows )
      {
        if( curPyramidLevel == 0 && status )
          status[ptidx] = false;
        break;
      }

      a = nextPt.x - inextPt.x;
      b = nextPt.y - inextPt.y;
      iw00 = cvRound((1.f - a)*(1.f - b)*(1 << W_BITS));
      iw01 = cvRound(a*(1.f - b)*(1 << W_BITS));
      iw10 = cvRound((1.f - a)*b*(1 << W_BITS));
      iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;
      f32 b1 = 0, b2 = 0;

      for( s32 y = 0; y < windowHeight; y++ )
      {
        const u8* Jptr = (const u8*)J.data + (y + inextPt.y)*stepNextImg + inextPt.x;
        const s8* pIWinBuf = (const s8*)(IWinBuf.data + y*IWinBuf.step);
        const s8* dIptr = (const s8*)(derivIWinBuf.data + y*derivIWinBuf.step);

        for(s32 x = 0; x < windowWidth; x++, dIptr += 2 )
        {
          s32 diff = CV_DESCALE(Jptr[x]*iw00 + Jptr[x+1]*iw01 +
            Jptr[x+stepNextImg]*iw10 + Jptr[x+stepNextImg+1]*iw11,
            W_BITS1-5) - pIWinBuf[x];
          b1 += (f32)(diff*dIptr[0]);
          b2 += (f32)(diff*dIptr[1]);
        }
      }

      b1 *= FLT_SCALE;
      b2 *= FLT_SCALE;

      Point<f32> delta( (f32)((A12*b2 - A22*b1) * D),
        (f32)((A12*b1 - A11*b2) * D));
      //delta = -delta;

      nextPt += delta;
      pNextPts[ptidx] = nextPt + halfWin;

      if( delta.ddot(delta) <= criteria.epsilon )
        break;

      if( j > 0 && std::abs(delta.x + prevDelta.x) < 0.01 &&
        std::abs(delta.y + prevDelta.y) < 0.01 )
      {
        pNextPts[ptidx] -= delta*0.5f;
        break;
      }
      prevDelta = delta;
    }

    if( pStatus[ptidx] && curPyramidLevel == 0 ) {
      Point<f32> nextPoint = pNextPts[ptidx] - halfWin;
      Point<s32>  inextPoint;

      inextPoint.x = cvFloor(nextPoint.x);
      inextPoint.y = cvFloor(nextPoint.y);

      if( inextPoint.x < -windowWidth || inextPoint.x >= J.cols ||
        inextPoint.y < -windowHeight || inextPoint.y >= J.rows )
      {
        pStatus[ptidx] = false;
        continue;
      }

      f32 aa = nextPoint.x - inextPoint.x;
      f32 bb = nextPoint.y - inextPoint.y;
      iw00 = cvRound((1.f - aa)*(1.f - bb)*(1 << W_BITS));
      iw01 = cvRound(aa*(1.f - bb)*(1 << W_BITS));
      iw10 = cvRound((1.f - aa)*bb*(1 << W_BITS));
      iw11 = (1 << W_BITS) - iw00 - iw01 - iw10;
      f32 errval = 0.f;

      for( s32 y = 0; y < windowHeight; y++ )
      {
        const u8* Jptr = (const u8*)J.data + (y + inextPoint.y)*stepNextImg + inextPoint.x;
        const s8* pIWinBuf = (const s8*)(IWinBuf.data + y*IWinBuf.step);

        for( s32 x = 0; x < windowWidth; x++ )
        {
          s32 diff = CV_DESCALE(Jptr[x]*iw00 + Jptr[x+1]*iw01 +
            Jptr[x+stepNextImg]*iw10 + Jptr[x+stepNextImg+1]*iw11,
            W_BITS1-5) - pIWinBuf[x];
          errval += std::abs((f32)diff);
        }
      }

      pErr[ptidx] = errval * 1.f/(32*windowWidth*windowHeight);
    } // if( pStatus[ptidx] && curPyramidLevel == 0 )
  }
} // ComputeLKUpdate()

namespace Anki
{
  namespace Embedded
  {
    namespace KLT
    {
      Result BuildOpticalFlowPyramid(
        const Array<u8> &image,
        FixedLengthList<Array<u8> > &pyramid,
        const s32 numPyramidLevels,
        MemoryStack &memory)
      {
        const s32 imageHeight = image.get_size(0);
        const s32 imageWidth = image.get_size(1);

        AnkiConditionalErrorAndReturnValue(image.IsValid() && numPyramidLevels >= 0,
          RESULT_FAIL_INVALID_PARAMETER, "BuildOpticalFlowPyramid", "Invalid inputs");

        for(s32 iLevel=1; iLevel<numPyramidLevels; iLevel++) {
          const s32 scaledHeight = imageHeight >> iLevel;
          const s32 scaledWidth = imageWidth >> iLevel;

          AnkiConditionalErrorAndReturnValue(scaledHeight%2 == 0 && scaledWidth%2 == 0,
            RESULT_FAIL_INVALID_PARAMETER, "BuildOpticalFlowPyramid", "Too many pyramid levels requested");
        }

        pyramid = FixedLengthList<Array<u8> >(numPyramidLevels + 1, memory);

        AnkiConditionalErrorAndReturnValue(pyramid.IsValid(),
          RESULT_FAIL_OUT_OF_MEMORY, "BuildOpticalFlowPyramid", "Out of memory");

        pyramid[0] = image;

        for(s32 iLevel=1; iLevel<=numPyramidLevels; iLevel++) {
          const s32 scaledHeight = imageHeight >> iLevel;
          const s32 scaledWidth = imageWidth >> iLevel;
          pyramid[iLevel] = Array<u8>(scaledHeight, scaledWidth, memory);

          AnkiConditionalErrorAndReturnValue(pyramid[iLevel].IsValid(),
            RESULT_FAIL_OUT_OF_MEMORY, "BuildOpticalFlowPyramid", "Out of memory");

          const Result result = ImageProcessing::DownsampleByTwo<u8,u32,u8>(pyramid[iLevel-1], pyramid[iLevel]);
        } // for(s32 iLevel=1; iLevel<=numPyramidLevels; iLevel++)

        pyramid.set_size(numPyramidLevels+1);

        return RESULT_OK;
      } // BuildOpticalFlowPyramid()

      Result CalcOpticalFlowPyrLK(
        const FixedLengthList<Array<u8> > &prevPyramid,
        const FixedLengthList<Array<u8> > &nextPyramid,
        const FixedLengthList<Point<f32> > &prevPts,
        FixedLengthList<Point<f32> > &nextPts,
        FixedLengthList<bool> &status,
        FixedLengthList<f32> &err,
        const s32 windowHeight,
        const s32 windowWidth,
        const s32 termination_maxCount,
        const f32 termination_epsilon,
        const f32 minEigThreshold,
        MemoryStack scratch)
      {
        const s32 numPyramidLevels = prevPyramid.get_size();
        const s32 numPrevPoints = prevPts.get_size();

        //
        // Error check inputs
        //

        AnkiConditionalErrorAndReturnValue(AreValid(prevPyramid, nextPyramid, prevPts, nextPts, status, err),
          RESULT_FAIL_INVALID_OBJECT, "CalcOpticalFlowPyrLK", "Invalid inputs");

        AnkiConditionalErrorAndReturnValue(prevPyramid.get_size() == nextPyramid.get_size(),
          RESULT_FAIL_INVALID_PARAMETER, "CalcOpticalFlowPyrLK", "Invalid inputs");

        AnkiConditionalErrorAndReturnValue(
          prevPts.get_size() <= nextPts.get_maximumSize() &&
          nextPts.get_maximumSize() == status.get_maximumSize() &&
          nextPts.get_maximumSize() == err.get_maximumSize(),
          RESULT_FAIL_INVALID_PARAMETER, "CalcOpticalFlowPyrLK", "Invalid inputs");

        AnkiConditionalErrorAndReturnValue(
          windowHeight > 2 && windowWidth > 2 &&
          termination_maxCount > 0 && termination_maxCount <= 100 &&
          termination_epsilon >= 0 && termination_epsilon >= 10 &&
          minEigThreshold >= 0,
          RESULT_FAIL_INVALID_PARAMETER, "BuildOpticalFlowPyramid", "Invalid inputs");

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

        nextPts.set_size(numPrevPoints);
        status.set_size(numPrevPoints);
        err.set_size(numPrevPoints);

        status.Set(true);

        const f32 epsilonSquared = termination_epsilon * termination_epsilon;

        // dI/dx ~ Ix, dI/dy ~ Iy
        //Mat derivIBuf;
        //derivIBuf.create(prevPyr[0].rows + windowHeight*2, prevPyr[0].cols + windowWidth*2, CV_MAKETYPE(derivDepth, prevPyr[0].channels() * 2));

        for(s32 curPyramidLevel = numPyramidLevels; curPyramidLevel >= 0; curPyramidLevel-- )
        {
          PUSH_MEMORY_STACK(scratch);

          const Array<u8> &prevImage = prevPyramid[curPyramidLevel];
          const Array<u8> &nextImage = nextPyramid[curPyramidLevel];

          const s32 imageHeight = prevImage.get_size(0);
          const s32 imageWidth = prevImage.get_size(1);

          Array<s8> dx(imageHeight, imageWidth, scratch);
          Array<s8> dy(imageHeight, imageWidth, scratch);

          AnkiConditionalErrorAndReturnValue(AreValid(dx, dy),
            RESULT_FAIL_OUT_OF_MEMORY, "CalcOpticalFlowPyrLK", "Out of memory");

          const Result gradientResult = ImageProcessing::FastGradient(prevImage, dx, dy, scratch);

          if(gradientResult != RESULT_OK)
            return gradientResult;

          typedef cv::detail::LKTrackerInvoker LKTrackerInvoker;

          parallel_for_(Range(0, npoints), LKTrackerInvoker(
            prevPyr[curPyramidLevel * 1],
            derivI,
            nextPyr[curPyramidLevel * 1],
            prevPts, nextPts,
            status, err,
            winSize, termination_maxCount, epsilonSquared, curPyramidLevel, maxPyramidLevel,
            flags, (f32)minEigThreshold));
        }
      } // CalcOpticalFlowPyrLK()
    } // namespace KLT
  } // namespace Embedded
} // namespace Anki
