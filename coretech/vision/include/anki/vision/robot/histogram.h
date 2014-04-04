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

      // Automatically compute some statistics when computing the histogram
      s32 numElements; //< The sum of all bins
      s32 min; //< The absolute min (0th percentile)
      s32 max; //< The absolute max (100th percentile)
      s32 sum; //< The sum of the values of all elements
      f32 mean; //< The mean of the values of all elements
      f32 standardDeviation; //< The unbiased standard deviation of the values of all elements

      Histogram();
      Histogram(const s32 numBins, MemoryStack &memory);

      void Reset();

      Result Set(const Histogram &in);

      s32 get_numBins() const;
    } Histogram;

    // Counts the number of pixels for each 0-255 grayvalue, and returns a list of the counts
    Result ComputeHistogram(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, Histogram &histogram);
    Histogram ComputeHistogram(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory);

    // Based on an input histogram, what is the bin index of a percentile [0,1]
    // For example, the .1 percentile is the value where 10% of the histogram energy is less than the value
    s32 ComputePercentile(const Histogram &histogram, const f32 percentile);
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_HISTOGRAM_H_
