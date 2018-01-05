/**
File: interpolate.h
Author: Peter Barnum
Created: 2013

Definitions of interpolate_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_H_
#define _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_H_

#include "coretech/common/robot/interpolate_declarations.h"

#include "coretech/common/robot/sequences.h"
#include "coretech/common/robot/geometry.h"
#include "coretech/common/robot/comparisons.h"

namespace Anki
{
  namespace Embedded
  {
    namespace InterpolateOperation
    {
      // const Type h00, h01, h02, h10, h11, h12;
      template<typename Type> Affine<Type>::Affine(const Array<Type> &homography)
        : h00(homography[0][0]), h01(homography[0][1]), h02(homography[0][2]),
        h10(homography[1][0]), h11(homography[1][1]), h12(homography[1][2])
      {
      }

      template<typename Type> inline void Affine<Type>::Apply(const Type inX, const Type inY, Type &outX, Type &outY)
      {
        outX = h00*inX + h01*inY + h02;
        outY = h10*inX + h11*inY + h12;
      }
    }

    template<typename Type> inline Type InterpolateBilinear2d(const Type pixelTL, const Type pixelTR, const Type pixelBL, const Type pixelBR, const Type alphaY, const Type alphaYinverse, const Type alphaX, const Type alphaXinverse)
    {
      const Type interpolatedTop = alphaXinverse*pixelTL + alphaX*pixelTR;
      const Type interpolatedBottom = alphaXinverse*pixelBL + alphaX*pixelBR;
      const Type interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;

      return interpolatedPixel;
    }

    template<typename InType, typename OutType> Result Interp2(const Array<InType> &reference, const Array<f32> &xCoordinates, const Array<f32> &yCoordinates, Array<OutType> &out, const InterpolationType interpolationType, const OutType invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_LINEAR,
        RESULT_FAIL_INVALID_PARAMETER, "Interp2", "Only INTERPOLATE_LINEAR is supported");

      AnkiConditionalErrorAndReturnValue(AreValid(reference, xCoordinates, yCoordinates, out),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "Invalid objects");

      const s32 referenceHeight = reference.get_size(0);
      const s32 referenceWidth = reference.get_size(1);

      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);

      const s32 numOutputElements = outHeight * outWidth;

      const bool isOutputOneDimensional = (out.get_size(0) == 1);

      AnkiConditionalErrorAndReturnValue(
        AreEqualSize(out, xCoordinates, yCoordinates),
        RESULT_FAIL_INVALID_SIZE, "Interp2", "xCoordinates, yCoordinates, and out must all be the same sizes");

      if(isOutputOneDimensional) {
        AnkiConditionalErrorAndReturnValue(
          AreEqualSize(1, numOutputElements, out),
          RESULT_FAIL_INVALID_SIZE, "Interp2", "If out is a row vector, then out, xCoordinates, and yCoordinates must all be 1xN");
      }

      AnkiConditionalErrorAndReturnValue(
        NotAliased(out, xCoordinates, yCoordinates, reference),
        RESULT_FAIL_ALIASED_MEMORY, "Interp2", "xCoordinates, yCoordinates, and reference cannot be the same as out");

      const f32 xyReferenceMin = 0.0f;
      const f32 xReferenceMax = static_cast<f32>(referenceWidth) - 1.0f;
      const f32 yReferenceMax = static_cast<f32>(referenceHeight) - 1.0f;

      //const s32 numValues = xCoordinates.get_size(1);

      const s32 yIterationMax = isOutputOneDimensional ? 1                    : outHeight;
      const s32 xIterationMax = isOutputOneDimensional ? (outHeight*outWidth) : outWidth;

      for(s32 y=0; y<yIterationMax; y++) {
        const f32 * restrict pXCoordinates = xCoordinates.Pointer(y,0);
        const f32 * restrict pYCoordinates = yCoordinates.Pointer(y,0);

        OutType * restrict pOut = out.Pointer(y,0);

        for(s32 x=0; x<xIterationMax; x++) {
          const f32 curX = pXCoordinates[x];
          const f32 curY = pYCoordinates[x];

          const f32 x0 = floorf(curX);
          const f32 x1 = ceilf(curX); // x0 + 1.0f;

          const f32 y0 = floorf(curY);
          const f32 y1 = ceilf(curY); // y0 + 1.0f;

          // If out of bounds, set as invalid and continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            pOut[x] = invalidValue;
            continue;
          }

          const f32 alphaX = curX - x0;
          const f32 alphaXinverse = 1 - alphaX;

          const f32 alphaY = curY - y0;
          const f32 alphaYinverse = 1.0f - alphaY;

          const s32 y0S32 = Round<s32>(y0);
          const s32 y1S32 = Round<s32>(y1);
          const s32 x0S32 = Round<s32>(x0);

          const InType * restrict pReference_y0 = reference.Pointer(y0S32, x0S32);
          const InType * restrict pReference_y1 = reference.Pointer(y1S32, x0S32);

          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);

          const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

          const OutType interpolatedPixel = RoundIfInteger<OutType>(interpolatedPixelF32);

          pOut[x] = interpolatedPixel;
        } // for(s32 x=0; x<xIterationMax; x++)
      } // for(s32 y=0; y<yIterationMax; y++)

      return RESULT_OK;
    } // Interp2

    template<typename InType, typename OutType> Result Interp2_Affine(const Array<InType> &reference, const Meshgrid<f32> &originalCoordinates, const Array<f32> &homography, const Point<f32> &centerOffset, Array<OutType> &out, const InterpolationType interpolationType, const OutType invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_LINEAR,
        RESULT_FAIL_INVALID_PARAMETER, "Interp2_Affine", "Only INTERPOLATE_LINEAR is supported");

      AnkiConditionalErrorAndReturnValue(AreValid(reference, out),
        RESULT_FAIL_INVALID_OBJECT, "Interp2_Affine", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(NotAliased(reference, out),
        RESULT_FAIL_ALIASED_MEMORY, "Interp2_Affine", "reference cannot be the same as out");

      const s32 referenceHeight = reference.get_size(0);
      const s32 referenceWidth = reference.get_size(1);

      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);

      const bool isOutputOneDimensional = (out.get_size(0) == 1);

      const f32 xyReferenceMin = 0.0f;
      const f32 xReferenceMax = static_cast<f32>(referenceWidth) - 1.0f;
      const f32 yReferenceMax = static_cast<f32>(referenceHeight) - 1.0f;

      const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
      const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];

      const LinearSequence<f32> &yGridVector = originalCoordinates.get_yGridVector();
      const LinearSequence<f32> &xGridVector = originalCoordinates.get_xGridVector();

      const f32 yGridStart = yGridVector.get_start();
      const f32 xGridStart = xGridVector.get_start();

      const f32 yGridDelta = yGridVector.get_increment();
      const f32 xGridDelta = xGridVector.get_increment();

      const s32 yIterationMax = yGridVector.get_size();
      const s32 xIterationMax = xGridVector.get_size();

      const f32 yTransformedDelta = h10 * yGridDelta;
      const f32 xTransformedDelta = h00 * xGridDelta;

      // One last check, to see if the sizes match
      if(isOutputOneDimensional) {
        const s32 numOutputElements = outHeight * outWidth;
        const s32 numOriginalCoordinates = xGridVector.get_size() * yGridVector.get_size();

        AnkiConditionalErrorAndReturnValue(
          outWidth == numOutputElements &&
          numOriginalCoordinates == numOutputElements,
          RESULT_FAIL_INVALID_SIZE, "Interp2_Affine", "originalCoordinates is the wrong size");
      } else {
        AnkiConditionalErrorAndReturnValue(
          yGridVector.get_size() == outHeight &&
          xGridVector.get_size() == outWidth,
          RESULT_FAIL_INVALID_SIZE, "Interp2_Affine", "originalCoordinates is the wrong size");
      }

      OutType * restrict pOut = out.Pointer(0,0);

      if(isOutputOneDimensional) {
        // pOut is incremented at the top of the loop, so decrement it here
        pOut -= xIterationMax;
      }

      f32 yOriginal = yGridStart;
      for(s32 y=0; y<yIterationMax; y++) {
        if(isOutputOneDimensional) {
          // If the output is one dimensional, then we will do the next set of x iterations later on
          // the same output row
          pOut += xIterationMax;
        } else {
          pOut = out.Pointer(y,0);
        }

        const f32 xOriginal = xGridStart;

        // TODO: This could be strength-reduced further, but it wouldn't be much faster
        f32 xTransformed = h00*xOriginal + h01*yOriginal + h02 + centerOffset.x;
        f32 yTransformed = h10*xOriginal + h11*yOriginal + h12 + centerOffset.y;

        for(s32 x=0; x<xIterationMax; x++) {
          const f32 x0 = floorf(xTransformed);
          const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

          const f32 y0 = floorf(yTransformed);
          const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

          // If out of bounds, set as invalid and continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            // strength reduction for the affine transformation along this horizontal line
            xTransformed += xTransformedDelta;
            yTransformed += yTransformedDelta;

            pOut[x] = invalidValue;
            continue;
          }

          const f32 alphaX = xTransformed - x0;
          const f32 alphaXinverse = 1 - alphaX;

          const f32 alphaY = yTransformed - y0;
          const f32 alphaYinverse = 1.0f - alphaY;

          const s32 y0S32 = Round<s32>(y0);
          const s32 y1S32 = Round<s32>(y1);
          const s32 x0S32 = Round<s32>(x0);

          const InType * restrict pReference_y0 = reference.Pointer(y0S32, x0S32);
          const InType * restrict pReference_y1 = reference.Pointer(y1S32, x0S32);

          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);

          const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

          const OutType interpolatedPixel = RoundIfInteger<OutType>(interpolatedPixelF32);

          pOut[x] = interpolatedPixel;

          // strength reduction for the affine transformation along this horizontal line
          xTransformed += xTransformedDelta;
          yTransformed += yTransformedDelta;
        } // for(s32 x=0; x<xIterationMax; x++)

        yOriginal += yGridDelta;
      } // for(s32 y=0; y<yIterationMax; y++)

      return RESULT_OK;
    } // Interp2_Affine

    template<typename InType, typename OutType> Result Interp2_Projective(const Array<InType> &reference, const Meshgrid<f32> &originalCoordinates, const Array<f32> &homography, const Point<f32> &centerOffset, Array<OutType> &out, const InterpolationType interpolationType, const OutType invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_LINEAR,
        RESULT_FAIL_INVALID_PARAMETER, "Interp2_Projective", "Only INTERPOLATE_LINEAR is supported");

      AnkiConditionalErrorAndReturnValue(AreValid(reference, out),
        RESULT_FAIL_INVALID_OBJECT, "Interp2_Projective", "Invalid objects");

      AnkiConditionalErrorAndReturnValue(NotAliased(reference, out),
        RESULT_FAIL_ALIASED_MEMORY, "Interp2_Projective", "reference cannot be the same as out");

      //AnkiConditionalErrorAndReturnValue(FLT_NEAR(homography[2][2], 1.0f),
      //  RESULT_FAIL_INVALID_PARAMETER, "Interp2_Projective", "homography[2][2] should be 1.0");

      const s32 referenceHeight = reference.get_size(0);
      const s32 referenceWidth = reference.get_size(1);

      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);

      const bool isOutputOneDimensional = (out.get_size(0) == 1);

      const f32 xyReferenceMin = 0.0f;
      const f32 xReferenceMax = static_cast<f32>(referenceWidth) - 1.0f;
      const f32 yReferenceMax = static_cast<f32>(referenceHeight) - 1.0f;

      const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
      const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
      const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = homography[2][2];

      const LinearSequence<f32> &yGridVector = originalCoordinates.get_yGridVector();
      const LinearSequence<f32> &xGridVector = originalCoordinates.get_xGridVector();

      const f32 yGridStart = yGridVector.get_start();
      const f32 xGridStart = xGridVector.get_start();

      const f32 yGridDelta = yGridVector.get_increment();
      const f32 xGridDelta = xGridVector.get_increment();

      const s32 yIterationMax = yGridVector.get_size();
      const s32 xIterationMax = xGridVector.get_size();

      // One last check, to see if the sizes match
      if(isOutputOneDimensional) {
        const s32 numOutputElements = outHeight * outWidth;
        const s32 numOriginalCoordinates = xGridVector.get_size() * yGridVector.get_size();

        AnkiConditionalErrorAndReturnValue(
          outWidth == numOutputElements &&
          numOriginalCoordinates == numOutputElements,
          RESULT_FAIL_INVALID_SIZE, "Interp2_Projective", "originalCoordinates is the wrong size");
      } else {
        AnkiConditionalErrorAndReturnValue(
          yGridVector.get_size() == outHeight &&
          xGridVector.get_size() == outWidth,
          RESULT_FAIL_INVALID_SIZE, "Interp2_Projective", "originalCoordinates is the wrong size");
      }

      OutType * restrict pOut = out.Pointer(0,0);

      if(isOutputOneDimensional) {
        // pOut is incremented at the top of the loop, so decrement it here
        pOut -= xIterationMax;
      }

      f32 yOriginal = yGridStart;
      for(s32 y=0; y<yIterationMax; y++) {
        if(isOutputOneDimensional) {
          // If the output is one dimensional, then we will do the next set of x iterations later on
          // the same output row
          pOut += xIterationMax;
        } else {
          pOut = out.Pointer(y,0);
        }

        f32 xOriginal = xGridStart;

        for(s32 x=0; x<xIterationMax; x++) {
          // TODO: These two could be strength reduced
          const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
          const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

          const f32 normalization = h20*xOriginal + h21*yOriginal + h22;

          const f32 xTransformed = (xTransformedRaw / normalization) + centerOffset.x;
          const f32 yTransformed = (yTransformedRaw / normalization) + centerOffset.y;

          xOriginal += xGridDelta;

          const f32 x0 = floorf(xTransformed);
          const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

          const f32 y0 = floorf(yTransformed);
          const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

          // If out of bounds, set as invalid and continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            pOut[x] = invalidValue;
            continue;
          }

          const f32 alphaX = xTransformed - x0;
          const f32 alphaXinverse = 1 - alphaX;

          const f32 alphaY = yTransformed - y0;
          const f32 alphaYinverse = 1.0f - alphaY;

          const s32 y0S32 = Round<s32>(y0);
          const s32 y1S32 = Round<s32>(y1);
          const s32 x0S32 = Round<s32>(x0);

          const InType * restrict pReference_y0 = reference.Pointer(y0S32, x0S32);
          const InType * restrict pReference_y1 = reference.Pointer(y1S32, x0S32);

          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);

          const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

          const OutType interpolatedPixel = RoundIfInteger<OutType>(interpolatedPixelF32);

          pOut[x] = interpolatedPixel;
        } // for(s32 x=0; x<xIterationMax; x++)

        yOriginal += yGridDelta;
      } // for(s32 y=0; y<yIterationMax; y++)

      return RESULT_OK;
    } // Interp2_Projective

    // #pragma mark --- Specializations ---
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_H_
