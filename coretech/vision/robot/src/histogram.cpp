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
    IntegerCounts::IntegerCounts()
      : counts(FixedLengthList<s32>()), numElements(-1)
    {
    }

    IntegerCounts::IntegerCounts(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory)
      : counts(FixedLengthList<s32>()), numElements(-1)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      this->counts = FixedLengthList<s32>(256, memory);
      this->counts.set_size(this->counts.get_maximumSize());

      AnkiConditionalErrorAndReturn(this->counts.IsValid(),
        "IntegerCounts::IntegerCounts", "Out of memory");

      this->counts.set_size(this->counts.get_maximumSize());
      this->counts.SetZero();

      s32 * restrict pCounts = this->counts.Pointer(0);

      const Rectangle<s32> imageRegionOfInterestClipped(
        MAX(0, MIN(imageWidth, imageRegionOfInterest.left)),
        MAX(0, MIN(imageWidth, imageRegionOfInterest.right)),
        MAX(0, MIN(imageHeight, imageRegionOfInterest.top)),
        MAX(0, MIN(imageHeight, imageRegionOfInterest.bottom)));

      this->numElements = 0;

      for(s32 y=imageRegionOfInterestClipped.top; y<=(imageRegionOfInterestClipped.bottom-1); y+=yIncrement) {
        const u8 * restrict pImage = image.Pointer(y,0);

        for(s32 x=imageRegionOfInterestClipped.left; x<=(imageRegionOfInterestClipped.right-1); x+=xIncrement) {
          const u8 curImage = pImage[x];
          pCounts[curImage]++;
          this->numElements++;
        }
      }

      // TODO: fix for speed
      /*const s32 numY = ((imageRegionOfInterest.bottom - imageRegionOfInterest.top) / yIncrement) + 1;
      const s32 numX = ((imageRegionOfInterest.right - imageRegionOfInterest.left) / xIncrement) + 1;
      this->numElements = numY * numX;

      CoreTechPrint("%d %d", this->numElements, numPoints);*/
    }

    s32 IntegerCounts::ComputePercentile(const f32 percentile) const
    {
      // TODO: there may be a boundary error in this function

      s32 binIndex = 0;
      s32 count = 0;

      const s32 numBins = this->get_numBins();

      const s32 * restrict pCounts = this->counts.Pointer(0);

      const s32 numBelow = Round<s32>(static_cast<f32>(this->numElements) * percentile);

      count += pCounts[0];

      while(count < numBelow && binIndex < (numBins-1)) {
        binIndex++;
        count += pCounts[binIndex];
      }

      return binIndex;
    }

    IntegerCounts::Statistics IntegerCounts::ComputeStatistics() const
    {
      IntegerCounts::Statistics statistics;

      statistics.isValid = false;

      statistics.numElements = this->numElements;

      const s32 numBins = this->get_numBins();

      const s32 * restrict pCounts = this->counts.Pointer(0);

      statistics.sum = 0;
      for(s32 i=0; i<numBins; i++) {
        statistics.sum += i * pCounts[i];
      }

      statistics.mean = static_cast<f32>(statistics.sum) / static_cast<f32>(this->numElements);

      f32 sumOfSquaredDifferences = 0;
      for(s32 i=0; i<numBins; i++) {
        const f32 diff = static_cast<f32>(i) - statistics.mean;
        sumOfSquaredDifferences += static_cast<f32>(pCounts[i]) * (diff*diff);
      }

      // Unbiased standard deviation
      statistics.standardDeviation = sqrtf( sumOfSquaredDifferences / static_cast<f32>(this->numElements - 1) );

      for(s32 i=0; i<numBins; i++) {
        if(pCounts[i] > 0) {
          statistics.min = i;
          break;
        };
      }

      for(s32 i=numBins-1; i>=0; i--) {
        if(pCounts[i] > 0) {
          statistics.max = i;
          break;
        };
      }

      statistics.isValid = true;

      return statistics;
    }

    Result IntegerCounts::Set(const IntegerCounts &in)
    {
      AnkiConditionalErrorAndReturnValue(in.counts.get_size() == this->counts.get_size(),
        RESULT_FAIL, "Histogram::Set", "counts must be allocated");

      this->counts.Set(in.counts);
      this->numElements = in.numElements;

      return RESULT_OK;
    }

    bool IntegerCounts::IsValid() const
    {
      if(numElements < 0)
        return false;

      return counts.IsValid();
    }

    const FixedLengthList<s32>& IntegerCounts::get_counts() const
    {
      return this->counts;
    }

    s32 IntegerCounts::get_numBins() const
    {
      return this->counts.get_size();
    }

    s32 IntegerCounts::get_numElements() const
    {
      return this->numElements;
    }
  } // namespace Embedded
} // namespace Anki
