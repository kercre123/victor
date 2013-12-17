/**
File: laplacianPeaks.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/miscVisionKernels.h"
#include "anki/vision/robot/imageProcessing.h"

using namespace std;

namespace Anki
{
  namespace Embedded
  {
    static Result lastResult;

    Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, FixedLengthList<Point<s16> > &peaks, MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(boundary.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "boundary is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "peaks is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.get_maximumSize() == 4,
        RESULT_FAIL_INVALID_PARAMETERS, "ComputeQuadrilateralsFromConnectedComponents", "Currently only four peaks supported");

      const s32 maximumTemporaryPeaks = boundary.get_size() / 3; // Worst case
      const s32 numSigmaFractionalBits = 8;
      const s32 numStandardDeviations = 3;

      const s32 boundaryLength = boundary.get_size();

      //sigma = boundaryLength/64;
      const s32 sigma = boundaryLength << (numSigmaFractionalBits-6); // SQ23.8

      // spacing about 1/4 of side-length
      //spacing = max(3, round(boundaryLength/16));
      const s32 spacing = MAX(3,  boundaryLength >> 4);

      //  stencil = [1 zeros(1, spacing-2) -2 zeros(1, spacing-2) 1];
      const s32 stencilLength = 1 + spacing - 2 + 1 + spacing - 2 + 1;
      FixedPointArray<s32> stencil(1, stencilLength, 0, scratch); // SQ31.0
      stencil.SetZero();
      *stencil.Pointer(0, 0) = 1;
      *stencil.Pointer(0, spacing-1) = -2;
      *stencil.Pointer(0, stencil.get_size(1)-1) = 1;

      //dg2 = conv(stencil, gaussian_kernel(sigma));
      FixedPointArray<s32> gaussian = ImageProcessing::Get1dGaussianKernel(sigma, numSigmaFractionalBits, numStandardDeviations, scratch); // SQ23.8
      FixedPointArray<s32> differenceOfGaussian(1, gaussian.get_size(1)+stencil.get_size(1)-1, numSigmaFractionalBits, scratch); // SQ23.8

      if((lastResult = ImageProcessing::Correlate1d(stencil, gaussian, differenceOfGaussian)) != RESULT_OK)
        return lastResult;

      FixedPointArray<s32> boundaryXFiltered(1, boundary.get_size(), numSigmaFractionalBits, scratch); // SQ23.8
      FixedPointArray<s32> boundaryYFiltered(1, boundary.get_size(), numSigmaFractionalBits, scratch); // SQ23.8

      if(!boundaryYFiltered.IsValid())
        return RESULT_FAIL_INVALID_OBJECT;

      //differenceOfGaussian.Print("differenceOfGaussian");

      //r_smooth = imfilter(boundary, dg2(:), 'circular');
      {
        PUSH_MEMORY_STACK(scratch);
        FixedPointArray<s32> boundaryX(1, boundary.get_size(), 0, scratch); // SQ31.0

        if(!boundaryX.IsValid())
          return RESULT_FAIL_INVALID_OBJECT;

        const Point<s16> * restrict pConstBoundary = boundary.Pointer(0);
        s32 * restrict pBoundaryX = boundaryX.Pointer(0,0);

        const s32 lengthBoundary = boundary.get_size();
        for(s32 i=0; i<lengthBoundary; i++) {
          pBoundaryX[i] = pConstBoundary[i].x;
        }

        if((lastResult = ImageProcessing::Correlate1dCircularAndSameSizeOutput(boundaryX, differenceOfGaussian, boundaryXFiltered)) != RESULT_OK)
          return lastResult;

        //boundaryX.Print("boundaryX");
        //boundaryXFiltered.Print("boundaryXFiltered");
      } // PUSH_MEMORY_STACK(scratch);

      {
        PUSH_MEMORY_STACK(scratch);
        FixedPointArray<s32> boundaryY(1, boundary.get_size(), 0, scratch); // SQ31.0

        if(!boundaryY.IsValid())
          return RESULT_FAIL_INVALID_OBJECT;

        const Point<s16> * restrict pConstBoundary = boundary.Pointer(0);
        s32 * restrict pBoundaryY = boundaryY.Pointer(0,0);

        const s32 lengthBoundary = boundary.get_size();
        for(s32 i=0; i<lengthBoundary; i++) {
          pBoundaryY[i] = pConstBoundary[i].y;
        }

        if((lastResult = ImageProcessing::Correlate1dCircularAndSameSizeOutput(boundaryY, differenceOfGaussian, boundaryYFiltered)) != RESULT_OK)
          return lastResult;

        //boundaryY.Print("boundaryY");
        //boundaryYFiltered.Print("boundaryYFiltered");
      } // PUSH_MEMORY_STACK(scratch);

      //r_smooth = sum(r_smooth.^2, 2);
      FixedPointArray<s32> boundaryFilteredAndCombined(1, boundary.get_size(), 2*numSigmaFractionalBits, scratch); // SQ15.16
      s32 * restrict pBoundaryFilteredAndCombined = boundaryFilteredAndCombined.Pointer(0,0);

      const s32 * restrict pConstBoundaryXFiltered = boundaryXFiltered.Pointer(0,0);
      const s32 * restrict pConstBoundaryYFiltered = boundaryYFiltered.Pointer(0,0);

      const s32 lengthBoundary = boundary.get_size();
      for(s32 i=0; i<lengthBoundary; i++) {
        //const s32 xSquared = (pConstBoundaryXFiltered[i] * pConstBoundaryXFiltered[i]) >> numSigmaFractionalBits; // SQ23.8
        //const s32 ySquared = (pConstBoundaryYFiltered[i] * pConstBoundaryYFiltered[i]) >> numSigmaFractionalBits; // SQ23.8
        const s32 xSquared = (pConstBoundaryXFiltered[i] * pConstBoundaryXFiltered[i]); // SQ31.0 (multiplied by 2^numSigmaFractionalBits)
        const s32 ySquared = (pConstBoundaryYFiltered[i] * pConstBoundaryYFiltered[i]); // SQ31.0 (multiplied by 2^numSigmaFractionalBits)

        pBoundaryFilteredAndCombined[i] = xSquared + ySquared;
      }

      FixedLengthList<s32> localMaxima(maximumTemporaryPeaks, scratch);

      const s32 * restrict pConstBoundaryFilteredAndCombined = boundaryFilteredAndCombined.Pointer(0,0);

      // Find local maxima -- these should correspond to the corners of the square.
      // NOTE: one of the comparisons is >= while the other is >, in order to
      // combat rare cases where we have two responses next to each other that are exactly equal.
      // localMaxima = find(r_smooth >= r_smooth([end 1:end-1]) & r_smooth > r_smooth([2:end 1]));

      if(pConstBoundaryFilteredAndCombined[0] > pConstBoundaryFilteredAndCombined[1] &&
        pConstBoundaryFilteredAndCombined[0] >= pConstBoundaryFilteredAndCombined[boundary.get_size()-1]) {
          localMaxima.PushBack(0);
      }

      for(s32 i=1; i<(lengthBoundary-1); i++) {
        if(pConstBoundaryFilteredAndCombined[i] > pConstBoundaryFilteredAndCombined[i+1] &&
          pConstBoundaryFilteredAndCombined[i] >= pConstBoundaryFilteredAndCombined[i-1]) {
            localMaxima.PushBack(i);
        }
      }

      if(pConstBoundaryFilteredAndCombined[boundary.get_size()-1] > pConstBoundaryFilteredAndCombined[0] &&
        pConstBoundaryFilteredAndCombined[boundary.get_size()-1] >= pConstBoundaryFilteredAndCombined[boundary.get_size()-2]) {
          localMaxima.PushBack(boundary.get_size()-1);
      }

      //boundaryFilteredAndCombined.Print("boundaryFilteredAndCombined");
      //for(s32 i=0; i<boundaryFilteredAndCombined.get_size(1); i++) {
      //  printf("%d\n", boundaryFilteredAndCombined[0][i]);
      //}

      //localMaxima.Print("localMaxima");

      const Point<s16> * restrict pConstBoundary = boundary.Pointer(0);

      //localMaxima.Print("localMaxima");

      // Select the index of the top 4 local maxima
      // TODO: make efficient
      // TODO: make work for numbers of peaks other than 4
      s32 * restrict pLocalMaxima = localMaxima.Pointer(0);
      s32 maximaValues[4];
      s32 maximaIndexes[4];
      for(s32 i=0; i<4; i++) {
        maximaValues[i] = s32_MIN;
        maximaIndexes[i] = -1;
      }

      const s32 numLocalMaxima = localMaxima.get_size();
      for(s32 iMax=0; iMax<4; iMax++) {
        for(s32 i=0; i<numLocalMaxima; i++) {
          const s32 localMaximaIndex = pLocalMaxima[i];
          if(pConstBoundaryFilteredAndCombined[localMaximaIndex] > maximaValues[iMax]) {
            maximaValues[iMax] = pConstBoundaryFilteredAndCombined[localMaximaIndex];
            maximaIndexes[iMax] = localMaximaIndex;
          }
        }
        //printf("Maxima %d/%d is #%d %d\n", iMax, 4, maximaIndexes[iMax], maximaValues[iMax]);
        pBoundaryFilteredAndCombined[maximaIndexes[iMax]] = s32_MIN;
      }

      // Copy the maxima to the output peaks, ordered by their original index order, so they are
      // still in clockwise or counterclockwise order
      peaks.Clear();

      bool whichUsed[4];
      for(s32 i=0; i<4; i++) {
        whichUsed[i] = false;
      }

      for(s32 iMax=0; iMax<4; iMax++) {
        s32 curMinIndex = -1;
        for(s32 i=0; i<4; i++) {
          if(!whichUsed[i] && maximaIndexes[i] >= 0) {
            if(curMinIndex == -1 || maximaIndexes[curMinIndex] > maximaIndexes[i]) {
              curMinIndex = i;
            }
          }
        }

        //if(maximaIndexes[iMax] >= 0) {
        if(curMinIndex >= 0) {
          whichUsed[curMinIndex] = true;
          peaks.PushBack(pConstBoundary[maximaIndexes[curMinIndex]]);
        }
      }

      //boundary.Print();
      //peaks.Print();

      return RESULT_OK;
    } // Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki