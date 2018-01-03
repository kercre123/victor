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

#include "coretech/common/robot/config.h"
#include "coretech/common/robot/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
    class IntegerCounts
    {
      // An IntegerCounts is a simple histogram that has one bin for every possible value. For
      // example, an IntegerCounts of an Array<u8> will have 256 bins.

    public:
      typedef struct Statistics
      {
        bool isValid; //< Is this struct of Statistics correctly computed?
        s32 numElements; //< The sum of all bins
        s32 min; //< The absolute min (0th percentile)
        s32 max; //< The absolute max (100th percentile)
        s32 sum; //< The sum of the values of all elements
        f32 mean; //< The mean of the values of all elements
        f32 standardDeviation; //< The unbiased standard deviation of the values of all elements
      } Statistics;

      IntegerCounts();

      // Counts the number of pixels for each 0-255 grayvalue
      IntegerCounts(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory);
      IntegerCounts(const Array<u8> &image, const Quadrilateral<f32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory);

      // What is the bin index of a percentile [0,1]?
      // For example, the .1 percentile is the value where 10% of the histogram energy is less than the value
      // If there's an error, it will return -1
      s32 ComputePercentile(const f32 percentile) const;

      // This operation should be quite fast for u8 IntegerCounts, so even if you only need one of
      // the statistics, computing them all probably isn't an issue.
      IntegerCounts::Statistics ComputeStatistics() const;

      Result Set(const IntegerCounts &in);

      bool IsValid() const;

      const FixedLengthList<s32>& get_counts() const;

      // For example, for u8, numBins = 256
      s32 get_numBins() const;

      // The sum of the values of all bins. For example, for a 320x240 image, numElements = 76800
      s32 get_numElements() const;

    protected:
      FixedLengthList<s32> counts;
      s32 numElements; //< The sum of all bins
    }; // class IntegerCounts
  } // namespace Embedded
} // namespace Anki

#endif //_ANKICORETECHEMBEDDED_VISION_HISTOGRAM_H_
