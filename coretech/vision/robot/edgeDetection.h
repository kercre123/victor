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

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/fixedLengthList.h"
#include "coretech/common/robot/geometry.h"

#include "coretech/vision/robot/histogram.h"
#include "coretech/vision/robot/transformations.h"

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

      Result Serialize(const char *objectName, SerializedBuffer &buffer) const;
      Result Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack &memory); // Updates the buffer pointer and length before returning

      s32 get_serializationSize() const;

#if defined(ANKICORETECH_EMBEDDED_USE_OPENCV) && ANKICORETECH_EMBEDDED_USE_OPENCV
      // Allocates the returned cv::Mat on the heap
      cv::Mat DrawIndexes(const f32 scale=1.0f)  const;

      static cv::Mat DrawIndexes(
        const s32 imageHeight, const s32 imageWidth,
        const FixedLengthList<Point<s16> > &indexPoints1,
        const FixedLengthList<Point<s16> > &indexPoints2,
        const FixedLengthList<Point<s16> > &indexPoints3,
        const FixedLengthList<Point<s16> > &indexPoints4,
        const f32 scale=1.0f);
#endif
    } EdgeLists;

    // Calculate local extrema (edges) in an image. Returns four list for the four directions of change.
    Result DetectBlurredEdges_GrayvalueThreshold(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists);
    Result DetectBlurredEdges_GrayvalueThreshold(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists);

    Result DetectBlurredEdges_DerivativeThreshold(const Array<u8> &image, const s32 combHalfWidth, const s32 combResponseThreshold, const s32 everyNLines, EdgeLists &edgeLists);
    Result DetectBlurredEdges_DerivativeThreshold(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const s32 combHalfWidth, const s32 combResponseThreshold, const s32 everyNLines, EdgeLists &edgeLists);

    u8 ComputeGrayvalueThreshold(
      const IntegerCounts &integerCounts,
      const f32 blackPercentile,  //< What percentile of histogram energy is black? (.1 is a good value)
      const f32 whitePercentile); //< What percentile of histogram energy is white? (.9 is a good value)

    u8 ComputeGrayvalueThreshold(
      const Array<u8> &image, //< Used to compute the IntegerCounts
      const Rectangle<s32> &imageRegionOfInterest, //< Used to compute the IntegerCounts
      const s32 yIncrement, //< Used to compute the IntegerCounts
      const s32 xIncrement, //< Used to compute the IntegerCounts
      const f32 blackPercentile, //< What percentile of histogram energy is black? (.1 is a good value)
      const f32 whitePercentile, //< What percentile of histogram energy is white? (.9 is a good value)
      MemoryStack scratch);
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_EDGE_DETECTION_H_
