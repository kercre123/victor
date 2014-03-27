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
    Histogram::Histogram()
      : counts(FixedLengthList<s32>()), numElements(-1)
    {
    }

    Histogram::Histogram(const s32 numBins, MemoryStack &memory)
      : counts(FixedLengthList<s32>(numBins, memory)), numElements(0)
    {
      this->counts.set_size(numBins);
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

      for(s32 y=imageRegionOfInterestClipped.top; y<=imageRegionOfInterestClipped.bottom; y+=yIncrement) {
        const u8 * restrict pImage = image.Pointer(y,0);

        for(s32 x=imageRegionOfInterestClipped.left; x<=imageRegionOfInterestClipped.right; x+=xIncrement) {
          const u8 curImage = pImage[x];
          pHistogram[curImage]++;
        }
      }

      const s32 numY = (imageRegionOfInterest.bottom - imageRegionOfInterest.top + 1) / yIncrement;
      const s32 numX = (imageRegionOfInterest.right - imageRegionOfInterest.left + 1) / xIncrement;
      histogram.numElements = numY * numX;

      return histogram;
    }

    s32 ComputePercentile(const Histogram &histogram, const f32 percentile)
    {
      // TODO: there may be a boundary error in this function

      s32 binIndex = 0;
      s32 count = 0;

      const s32 numBins = histogram.counts.get_maximumSize();

      const s32 * restrict pHistogram = histogram.counts.Pointer(0);

      const s32 numBelow = RoundS32(static_cast<f32>(histogram.numElements) * percentile);

      count += pHistogram[0];

      while(count < numBelow && binIndex < numBins) {
        binIndex++;
        count += pHistogram[binIndex];
      }

      return binIndex;
    }
  } // namespace Embedded
} // namespace Anki
