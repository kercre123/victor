/**
File: imageProcessing.h
Author: Peter Barnum
Created: 2013

Definitions for imageProcessing_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_
#define _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_

#include "anki/vision/robot/imageProcessing_declarations.h"

#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    namespace ImageProcessing
    {
      template<typename InType, typename IntermediateType, typename OutType> Result ComputeXGradient(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 imageHeight = in.get_size(0);
        const s32 imageWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid() && out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTrackerAffine::ComputeXGradient", "An input is not valid");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == imageHeight && out.get_size(1) == imageWidth,
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTrackerAffine::ComputeXGradient", "Image sizes don't match");

        for(s32 y=1; y<imageHeight-1; y++) {
          const InType * restrict pIn = in.Pointer(y,0);

          OutType * restrict pOut = out.Pointer(y,0);

          for(s32 x=1; x<imageWidth-1; x++) {
            pOut[x] = static_cast<IntermediateType>(pIn[x+1]) - static_cast<IntermediateType>(pIn[x-1]);
          }
        }

        return RESULT_OK;
      }

      template<typename InType, typename IntermediateType, typename OutType> Result ComputeYGradient(const Array<InType> &in, Array<OutType> &out)
      {
        const s32 imageHeight = in.get_size(0);
        const s32 imageWidth = in.get_size(1);

        AnkiConditionalErrorAndReturnValue(in.IsValid() && out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTrackerAffine::ComputeYGradient", "An input is not valid");

        AnkiConditionalErrorAndReturnValue(out.get_size(0) == imageHeight && out.get_size(1) == imageWidth,
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTrackerAffine::ComputeYGradient", "Image sizes don't match");

        for(s32 y=1; y<imageHeight-1; y++) {
          const InType * restrict pIn_ym1 = in.Pointer(y-1,0);
          const InType * restrict pIn_yp1 = in.Pointer(y+1,0);

          OutType * restrict pOut = out.Pointer(y,0);

          for(s32 x=1; x<imageWidth-1; x++) {
            pOut[x] = static_cast<IntermediateType>(pIn_yp1[x]) - static_cast<IntermediateType>(pIn_ym1[x]);
          }
        }

        return RESULT_OK;
      }
    } // namespace ImageProcessing
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_IMAGE_PROCESSING_H_
