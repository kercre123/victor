/**
File: interpolate.h
Author: Peter Barnum
Created: 2013

Utilities for interpolation

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_H_
#define _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_H_

#include "anki/common/robot/interpolate_declarations.h"

#include "anki/common/robot/sequences.h"

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

      template<typename Type> static inline void Affine<Type>::Apply(const Type inX, const Type inY, Type &outX, Type &outY)
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

      template<typename Type> static inline void Projective<Type>::Apply(const Type inX, const Type inY, Type &outX, Type &outY)
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
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_H_
