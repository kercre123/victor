/**
File: histogram.h
Author: Peter Barnum
Created: 2014-03-19

Compute histograms from an input image

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_HISTOGRAM_H_
#define _ANKICORETECHEMBEDDED_VISION_HISTOGRAM_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
    typedef struct Histogram
    {
      FixedLengthList<s32> counts;
      s32 numElements;

      Histogram();
      Histogram(const s32 numBins, MemoryStack &memory);
    } Histogram;

    // Counts the number of pixels for each 0-255 grayvalue, and returns a list of the counts
    Histogram ComputeHistogram(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory);

    // Based on an input histogram, what is the bin index of a percentile [0,1]
    // For example, the .1 percentile is the value where 10% of the histogram energy is less than the value
    s32 ComputePercentile(const Histogram &histogram, const f32 percentile);
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_HISTOGRAM_H_
