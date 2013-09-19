//
//  dataStructures.h
//  CoreTech_Common
//
//  Created by Peter Barnum on 8/7/13.
//  Copyright (c) 2013 Peter Barnum. All rights reserved.
//

#ifndef _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_
#define _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_

#include "anki/embeddedVision/config.h"
#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    // A 1d, run-length encoded piece of a 2d component
    class ConnectedComponentSegment
    {
    public:
      s16 xStart, xEnd, y;
      u16 id;

      ConnectedComponentSegment();

      ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y = -1, const u16 id = 0);

      void Print() const;

      bool operator== (const ConnectedComponentSegment &component2) const;
    }; // class ConnectedComponentSegment

    class FiducialMarker
    {
    public:
      Point<s32> upperLeft, upperRight, lowerLeft, lowerRight; // SQ 15.16
      s16 blockType, faceType;

      FiducialMarker();

      // All points are SQ 15.16
      FiducialMarker(const Point<s32> upperLeft, const Point<s32> upperRight, const Point<s32> lowerLeft, const Point<s32> lowerRight, const s16 blockType, const s16 faceType);

      void Print() const;
    }; // class FiducialMarker
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_
