/**
File: interpolate_declarations.h
Author: Peter Barnum
Created: 2013

Utilities for interpolation

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/sequences_declarations.h"
#include "coretech/common/robot/geometry_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace InterpolateOperation
    {
      // Functor for 6 degree-of-freedom (6-DOF) affine interpolation
      template<typename Type> class Affine {
      public:
        Affine(const Array<Type> &homography);

        inline void Apply(const Type inX, const Type inY, Type &outX, Type &outY);

      protected:
        const Type h00, h01, h02, h10, h11, h12;
      };

      // Functor for 8 degree-of-freedom (8-DOF) projective interpolation
      template<typename Type> class Projective {
      public:
        Projective(const Array<Type> &homography);

        inline void Apply(const Type inX, const Type inY, Type &outX, Type &outY);

      protected:
        const Type h00, h01, h02, h10, h11, h12, h20, h21; // h22 should be 1
      };
    }

    enum InterpolationType
    {
      INTERPOLATE_LINEAR
    };

    // Similar to Matlab Interp2, except the upper-left corner is (0,0) instead of (1,1)
    template<typename InType, typename OutType> Result Interp2(const Array<InType> &reference, const Array<f32> &xCoordinates, const Array<f32> &yCoordinates, Array<OutType> &out, const InterpolationType interpolationType=INTERPOLATE_LINEAR, const OutType invalidValue=static_cast<OutType>(0));

    // Same as Interp2, but uses an affine homography to compute the lookup coordinates
    template<typename InType, typename OutType> Result Interp2_Affine(const Array<InType> &reference, const Meshgrid<f32> &originalCoordinates, const Array<f32> &homography, const Point<f32> &centerOffset, Array<OutType> &out, const InterpolationType interpolationType=INTERPOLATE_LINEAR, const OutType invalidValue=static_cast<OutType>(0));

    // Same as Interp2, but uses an projective homography to compute the lookup coordinates
    template<typename InType, typename OutType> Result Interp2_Projective(const Array<InType> &reference, const Meshgrid<f32> &originalCoordinates, const Array<f32> &homography, const Point<f32> &centerOffset, Array<OutType> &out, const InterpolationType interpolationType=INTERPOLATE_LINEAR, const OutType invalidValue=static_cast<OutType>(0));

    // T = Top, B = Bottom, L = Left, R = Right
    template<typename Type> inline Type InterpolateBilinear2d(const Type pixelTL, const Type pixelTR, const Type pixelBL, const Type pixelBR, const Type alphaY, const Type alphaYinverse, const Type alphaX, const Type alphaXinverse);

    // TODO: implement lazy-evaluation version
    // template<typename Type, typename Operation> Result Interp2(const Array<Type> &reference, const Meshgrid<f32> &grid, Array<Type> &out);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_INTERPOLATE_DECLARATIONS_H_
