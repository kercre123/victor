/**
File: connectedComponents.h
Author: Peter Barnum
Created: 2013

Definitions of connectedComponents_declarations.h

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_

#include "anki/vision/robot/connectedComponents_declarations.h"

#include "anki/common/types.h"
#include "anki/common/robot/memory.h"
#include "anki/common/robot/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Definitions ---

    inline s64 ConnectedComponents::CompareConnectedComponentSegments(const ConnectedComponentSegment &a, const ConnectedComponentSegment &b)
    {
      // Wraps zero around to MAX_u16
      u16 idA = a.id;
      u16 idB = b.id;

      idA -= 1;
      idB -= 1;

      const s64 idDifference = static_cast<s64>(u16_MAX) * static_cast<s64>(u16_MAX) * (static_cast<s64>(idA) - static_cast<s64>(idB));
      const s64 yDifference = static_cast<s64>(u16_MAX) * (static_cast<s64>(a.y) - static_cast<s64>(b.y));
      const s64 xStartDifference = (static_cast<s64>(a.xStart) - static_cast<s64>(b.xStart));

      return idDifference + yDifference + xStartDifference;
    }

    inline const ConnectedComponentSegment* ConnectedComponents::Pointer(const s32 index) const
    {
      return components.Pointer(index);
    }

    inline const ConnectedComponentSegment& ConnectedComponents::operator[](const s32 index) const
    {
      return *components.Pointer(index);
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
