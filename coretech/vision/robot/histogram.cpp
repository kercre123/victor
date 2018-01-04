/**
File: histogram.cpp
Author: Peter Barnum
Created: 2014-03-19

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/histogram.h"

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

    IntegerCounts::IntegerCounts(const Array<u8> &image, const Quadrilateral<f32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory)
      : counts(FixedLengthList<s32>()), numElements(-1)
    {
      // Based off DrawFilledConvexQuadrilateral()

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      this->counts = FixedLengthList<s32>(256, memory);
      this->counts.set_size(this->counts.get_maximumSize());

      AnkiConditionalErrorAndReturn(this->counts.IsValid(),
        "IntegerCounts::IntegerCounts", "Out of memory");

      this->counts.set_size(this->counts.get_maximumSize());
      this->counts.SetZero();

      s32 * restrict pCounts = this->counts.Pointer(0);

      this->numElements = 0;

      const Rectangle<f32> boundingRect = imageRegionOfInterest.ComputeBoundingRectangle<f32>();
      const Quadrilateral<f32> sortedQuad = imageRegionOfInterest.ComputeClockwiseCorners<f32>();

      const f32 rect_y0 = boundingRect.top;
      const f32 rect_y1 = boundingRect.bottom;

      // For circular indexing
      Point<f32> corners[5];
      for(s32 i=0; i<4; i++) {
        corners[i] = sortedQuad[i];
      }
      corners[4] = sortedQuad[0];

      const s32 minYS32 = MAX(0,             Round<s32>(ceilf(rect_y0 - 0.5f)));
      const s32 maxYS32 = MIN(imageHeight-1, Round<s32>(floorf(rect_y1 - 0.5f)));
      const f32 minYF32 = minYS32 + 0.5f;
      const f32 maxYF32 = maxYS32 + 0.5f;
      const LinearSequence<f32> ys(minYF32, maxYF32);
      const s32 numYs = ys.get_size();

      f32 yF32 = ys.get_start();
      for(s32 iy=0; iy<numYs; iy+=yIncrement) {
        // Compute all intersections
        f32 minXF32 = FLT_MAX;
        f32 maxXF32 = FLT_MIN;
        for(s32 iCorner=0; iCorner<4; iCorner++) {
          if( (corners[iCorner].y < yF32 && corners[iCorner+1].y >= yF32) || (corners[iCorner+1].y < yF32 && corners[iCorner].y >= yF32) ) {
            const f32 dy = corners[iCorner+1].y - corners[iCorner].y;
            const f32 dx = corners[iCorner+1].x - corners[iCorner].x;

            const f32 alpha = (yF32 - corners[iCorner].y) / dy;

            const f32 xIntercept = corners[iCorner].x + alpha * dx;

            minXF32 = MIN(minXF32, xIntercept);
            maxXF32 = MAX(maxXF32, xIntercept);
          }
        } // for(s32 iCorner=0; iCorner<4; iCorner++)

        const s32 minXS32 = MAX(0,            Round<s32>(floorf(minXF32+0.5f)));
        const s32 maxXS32 = MIN(imageWidth-1, Round<s32>(floorf(maxXF32-0.5f)));

        const s32 yS32 = minYS32 + iy;
        const u8 * restrict pImage = image.Pointer(yS32, 0);
        for(s32 x=minXS32; x<=maxXS32; x+=xIncrement) {
          const u8 curImage = pImage[x];
          pCounts[curImage]++;
          this->numElements++;
        }

        yF32 += static_cast<f32>(yIncrement);
      } // for(s32 iy=0; iy<numYs; iy++)
    } // IntegerCounts::IntegerCounts(const Array<u8> &image, const Quadrilateral<f32> &imageRegionOfInterest, const s32 yIncrement, const s32 xIncrement, MemoryStack &memory)

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
