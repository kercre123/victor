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
      // Compute the [-1, 0, 1] gradient
      template<typename InType, typename IntermediateType, typename OutType> Result ComputeXGradient(const Array<InType> &in, Array<OutType> &out);
      template<typename InType, typename IntermediateType, typename OutType> Result ComputeYGradient(const Array<InType> &in, Array<OutType> &out);
    } // namespace ImageProcessing
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_DECLARATIONS_H_
