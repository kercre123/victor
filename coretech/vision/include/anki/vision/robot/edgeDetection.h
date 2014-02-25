/**
File: edgeDetection.h
Author: Peter Barnum
Created: 2014-02-12

Computes local edges from an image

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_EDGE_DETECTION_H_
#define _ANKICORETECHEMBEDDED_VISION_EDGE_DETECTION_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/geometry.h"

namespace Anki
{
  namespace Embedded
  {
    typedef struct
    {
      // These should be allocated by the user, before calling DetectBlurredEdges
      FixedLengthList<Point<s16> > xDecreasing;
      FixedLengthList<Point<s16> > xIncreasing;
      FixedLengthList<Point<s16> > yDecreasing;
      FixedLengthList<Point<s16> > yIncreasing;

      // These will be set by DetectBlurredEdges
      s32 imageHeight;
      s32 imageWidth;
    } EdgeLists;

    // Calculate local extrema (edges) in an image. Returns four list for the four directions of change.
    Result DetectBlurredEdges(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, EdgeLists &edgeLists);
    Result DetectBlurredEdges(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, EdgeLists &edgeLists);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_EDGE_DETECTION_H_
