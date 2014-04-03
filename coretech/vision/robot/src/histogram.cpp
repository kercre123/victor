/**
File: histogram.cpp
Author: Peter Barnum
Created: 2014-03-19

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/histogram.h"

namespace Anki
{
  namespace Embedded
  {
    static void UpdateHistogramStatistics_UnsignedInt(Histogram &histogram)
    {
      const s32 numBins = histogram.get_numBins();

      const s32 * restrict pHistogram = histogram.counts.Pointer(0);

      histogram.sum = 0;
      for(s32 i=0; i<numBins; i++) {
        histogram.sum += i * pHistogram[i];
      }

      histogram.mean = static_cast<f32>(histogram.sum) / static_cast<f32>(histogram.numElements);

      f32 sumOfSquaredDifferences = 0;
      for(s32 i=0; i<numBins; i++) {
        const f32 diff = static_cast<f32>(i) - histogram.mean;
        sumOfSquaredDifferences += static_cast<f32>(pHistogram[i]) * (diff*diff);
      }

      // Unbiased standard deviation
      histogram.standardDeviation = sqrtf( sumOfSquaredDifferences / static_cast<f32>(histogram.numElements - 1) );

      for(s32 i=0; i<numBins; i++) {
        if(pHistogram[i] > 0) {
          histogram.min = i;
          break;
        };
      }

      for(s32 i=numBins-1; i>=0; i--) {
        if(pHistogram[i] > 0) {
          histogram.max = i;
          break;
        };
      }
    }

    Histogram::Histogram()
      : counts(FixedLengthList<s32>()), numElements(-1)
    {
    }

    Histogram::Histogram(const s32 numBins, MemoryStack &memory)
      : counts(FixedLengthList<s32>(numBins, memory)), numElements(0)
    {
      this->counts.set_size(numBins);
    }

    s32 Histogram::get_numBins() const
    {
      return this->counts.get_size();
    }

    Histogram ComputeHistogram(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      Histogram histogram(256, memory);

      s32 * restrict pHistogram = histogram.counts.Pointer(0);

      const Rectangle<s32> imageRegionOfInterestClipped(
        MAX(0, MIN(imageWidth-1, imageRegionOfInterest.left)),
        MAX(0, MIN(imageWidth-1, imageRegionOfInterest.right)),
        MAX(0, MIN(imageHeight-1, imageRegionOfInterest.top)),
        MAX(0, MIN(imageHeight-1, imageRegionOfInterest.bottom)));

      histogram.numElements = 0;

      for(s32 y=imageRegionOfInterestClipped.top; y<=imageRegionOfInterestClipped.bottom; y+=yIncrement) {
        const u8 * restrict pImage = image.Pointer(y,0);

        for(s32 x=imageRegionOfInterestClipped.left; x<=imageRegionOfInterestClipped.right; x+=xIncrement) {
          const u8 curImage = pImage[x];
          pHistogram[curImage]++;
          histogram.numElements++;
        }
      }

      // TODO: fix for speed
      /*const s32 numY = ((imageRegionOfInterest.bottom - imageRegionOfInterest.top) / yIncrement) + 1;
      const s32 numX = ((imageRegionOfInterest.right - imageRegionOfInterest.left) / xIncrement) + 1;
      histogram.numElements = numY * numX;

      printf("%d %d", histogram.numElements, numPoints);*/

      UpdateHistogramStatistics_UnsignedInt(histogram);

      return histogram;
    }

    s32 ComputePercentile(const Histogram &histogram, const f32 percentile)
    {
      // TODO: there may be a boundary error in this function

      s32 binIndex = 0;
      s32 count = 0;

      const s32 numBins = histogram.get_numBins();

      const s32 * restrict pHistogram = histogram.counts.Pointer(0);

      const s32 numBelow = RoundS32(static_cast<f32>(histogram.numElements) * percentile);

      count += pHistogram[0];

      while(count < numBelow && binIndex < (numBins-1)) {
        binIndex++;
        count += pHistogram[binIndex];
      }

      return binIndex;
    }
  } // namespace Embedded
} // namespace Anki
