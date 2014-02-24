/**
File: docking.h
Author: Peter Barnum
Created: December 3, 2013

Algorithms for docking a robot with a block

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_DOCKING_H_
#define _ANKICORETECHEMBEDDED_VISION_DOCKING_H_

#include "anki/common/robot/config.h"

#include "anki/vision/robot/lucasKanade.h"
#include "anki/vision/robot/transformations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Docking
    {
      /* OLD
      // The input is a transform, and outputs are error signal terms for the controller.
      // rel_ is one of {related-to-, relative-, real-}
      // rad means radians
      Result ComputeDockingErrorSignal(const Transformations::PlanarTransformation_f32 &transform, const s32 horizontalTrackingResolution, const f32 blockMarkerWidthInMM, const f32 horizontalFocalLengthInMM, f32 &rel_x, f32 &rel_y, f32 &rel_rad, MemoryStack scratch);
      */

      // The input is a tracker, the outputs are error signal terms for the controller.
      // rel_ is one of {related-to-, relative-, real-}
      // rad means radians
      Result ComputeDockingErrorSignal(const Transformations::PlanarTransformation_f32 &transform,
        const s32 horizontalTrackingResolution,
        const f32 blockMarkerWidthInMM,
        const f32 horizontalFocalLengthInMM,
        f32& rel_x, f32& rel_y, f32& rel_rad,
        MemoryStack scratch);
    } // namespace Docking
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_DOCKING_H_
