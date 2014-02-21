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

#include "anki/common/robot/interpolate_declarations.h"

#include "anki/common/robot/sequences.h"
#include "anki/common/robot/geometry.h"

namespace Anki
{
  namespace Embedded
  {
    namespace InterpolateOperation
    {
      // #pragma mark

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

      // const Type h00, h01, h02, h10, h11, h12, h20, h21; // h22 should be 1
      template<typename Type> Projective<Type>::Projective(const Array<Type> &homography)
        : h00(homography[0][0]), h01(homography[0][1]), h02(homography[0][2]),
        h10(homography[1][0]), h11(homography[1][1]), h12(homography[1][2]),
        h20(homography[2][0]), h21(homography[2][1])
      {
        AnkiConditionalError(NEAR(homography[2][2], static_cast<Type>(1), 1e-10),
          "Projective<Type>::Projective", "homography[2][2] should equal 1");
      }

      template<typename Type> inline void Projective<Type>::Apply(const Type inX, const Type inY, Type &outX, Type &outY)
      {
        const Type normalization = h20*inX + h21*inY;

        outX = (h00*inX + h01*inY + h02) / normalization;
        outY = (h10*inX + h11*inY + h12) / normalization;
      }
    }

    template<typename Type> inline Type InterpolateBilinear2d(const Type pixelTL, const Type pixelTR, const Type pixelBL, const Type pixelBR, const Type alphaY, const Type alphaYinverse, const Type alphaX, const Type alphaXinverse)
    {
      const Type interpolatedTop = alphaXinverse*pixelTL + alphaX*pixelTR;
      const Type interpolatedBottom = alphaXinverse*pixelBL + alphaX*pixelBR;
      const Type interpolatedPixel = alphaYinverse*interpolatedTop + alphaY*interpolatedBottom;

      return interpolatedPixel;
    }

    template<typename TypeIn, typename TypeOut> Result Interp2(const Array<TypeIn> &reference, const Array<f32> &xCoordinates, const Array<f32> &yCoordinates, Array<TypeOut> &out, const InterpolationType interpolationType, const TypeOut invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_LINEAR,
        RESULT_FAIL_INVALID_PARAMETERS, "Interp2", "Only INTERPOLATE_LINEAR is supported");

      AnkiConditionalErrorAndReturnValue(reference.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "reference is not valid");

      AnkiConditionalErrorAndReturnValue(xCoordinates.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "xCoordinates is not valid");

      AnkiConditionalErrorAndReturnValue(yCoordinates.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "yCoordinates is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "out is not valid");

      const s32 referenceHeight = reference.get_size(0);
      const s32 referenceWidth = reference.get_size(1);

      const s32 outHeight = out.get_size(0);
      const s32 outWidth = out.get_size(1);

      const s32 numOutputElements = outHeight * outWidth;

      const bool isOutputOneDimensional = (out.get_size(0) == 1);

      if(isOutputOneDimensional) {
        AnkiConditionalErrorAndReturnValue(
          out.get_size(1) == numOutputElements && xCoordinates.get_size(1) == numOutputElements && yCoordinates.get_size(1) == numOutputElements &&
          xCoordinates.get_size(0) == 1 && yCoordinates.get_size(0) == 1,
          RESULT_FAIL_INVALID_SIZE, "Interp2", "If out is a row vector, then out, xCoordinates, and yCoordinates must all be 1xN");
      } else {
        AnkiConditionalErrorAndReturnValue(
          xCoordinates.get_size(0) == outHeight && xCoordinates.get_size(1) == outWidth &&
          yCoordinates.get_size(0) == outHeight && yCoordinates.get_size(1) == outWidth,
          RESULT_FAIL_INVALID_SIZE, "Interp2", "If the out is not 1 dimensional, then xCoordinates, yCoordinates, and out must all be the same sizes");
      }

      AnkiConditionalErrorAndReturnValue(xCoordinates.get_rawDataPointer() != out.get_rawDataPointer() &&
        yCoordinates.get_rawDataPointer() != out.get_rawDataPointer() &&
        reference.get_rawDataPointer() != out.get_rawDataPointer(),
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

        TypeOut * restrict pOut = out.Pointer(y,0);

        for(s32 x=0; x<xIterationMax; x++) {
          const f32 curX = pXCoordinates[x];
          const f32 curY = pYCoordinates[x];

          const f32 x0 = FLT_FLOOR(curX);
          const f32 x1 = ceilf(curX); // x0 + 1.0f;

          const f32 y0 = FLT_FLOOR(curY);
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

          const s32 y0S32 = static_cast<s32>(Round(y0));
          const s32 y1S32 = static_cast<s32>(Round(y1));
          const s32 x0S32 = static_cast<s32>(Round(x0));

          const TypeIn * restrict pReference_y0 = reference.Pointer(y0S32, x0S32);
          const TypeIn * restrict pReference_y1 = reference.Pointer(y1S32, x0S32);

          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);

          const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

          const TypeOut interpolatedPixel = static_cast<TypeOut>(interpolatedPixelF32);

          pOut[x] = interpolatedPixel;
        } // for(s32 x=0; x<xIterationMax; x++)
      } // for(s32 y=0; y<yIterationMax; y++)

      return RESULT_OK;
    }

    template<typename TypeIn, typename TypeOut> Result Interp2_Affine(const Array<TypeIn> &reference, const Meshgrid<f32> &originalCoordinates, const Array<f32> &homography, const Point<f32> &centerOffset, Array<TypeOut> &out, const InterpolationType interpolationType, const TypeOut invalidValue)
    {
      AnkiConditionalErrorAndReturnValue(interpolationType == INTERPOLATE_LINEAR,
        RESULT_FAIL_INVALID_PARAMETERS, "Interp2", "Only INTERPOLATE_LINEAR is supported");

      AnkiConditionalErrorAndReturnValue(reference.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "reference is not valid");

      AnkiConditionalErrorAndReturnValue(out.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "Interp2", "out is not valid");

      AnkiConditionalErrorAndReturnValue(reference.get_rawDataPointer() != out.get_rawDataPointer(),
        RESULT_FAIL_ALIASED_MEMORY, "Interp2", "reference cannot be the same as out");

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
          RESULT_FAIL_INVALID_SIZE, "Interp2", "originalCoordinates is the wrong size");
      } else {
        AnkiConditionalErrorAndReturnValue(
          yGridVector.get_size() == outHeight &&
          xGridVector.get_size() == outWidth,
          RESULT_FAIL_INVALID_SIZE, "Interp2", "originalCoordinates is the wrong size");
      }

      TypeOut * restrict pOut = out.Pointer(0,0);

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
          const f32 x0 = FLT_FLOOR(xTransformed);
          const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

          const f32 y0 = FLT_FLOOR(yTransformed);
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

          const s32 y0S32 = static_cast<s32>(Round(y0));
          const s32 y1S32 = static_cast<s32>(Round(y1));
          const s32 x0S32 = static_cast<s32>(Round(x0));

          const TypeIn * restrict pReference_y0 = reference.Pointer(y0S32, x0S32);
          const TypeIn * restrict pReference_y1 = reference.Pointer(y1S32, x0S32);

          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);

          const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);

          const TypeOut interpolatedPixel = static_cast<TypeOut>(interpolatedPixelF32);

          pOut[x] = interpolatedPixel;

          // strength reduction for the affine transformation along this horizontal line
          xTransformed += xTransformedDelta;
          yTransformed += yTransformedDelta;
        } // for(s32 x=0; x<xIterationMax; x++)

        yOriginal += yGridDelta;
      } // for(s32 y=0; y<yIterationMax; y++)

      return RESULT_OK;
    }

    // #pragma mark --- Specializations ---

    template<> Result Interp2(const Array<u8> &reference, const Array<f32> &xCoordinates, const Array<f32> &yCoordinates, Array<u8> &out, const InterpolationType interpolationType, const u8 invalidValue);

    template<> Result Interp2_Affine(const Array<u8> &reference, const Meshgrid<f32> &originalCoordinates, const Array<f32> &homography, const Point<f32> &centerOffset, Array<u8> &out, const InterpolationType interpolationType, const u8 invalidValue);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_H_
