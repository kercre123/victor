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
      Point<s16> corners[4]; // SQ 15.0 (Though may be changed later)
      Array<f64> homography;
      s16 blockType, faceType;

      FiducialMarker();

      void Print() const;

      FiducialMarker& operator= (const FiducialMarker &marker2);
    }; // class FiducialMarker
  } // namespace Embedded
} // namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_VISION_DATASTRUCTURES_VISION_H_
