/**
File: connectedComponents.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/connectedComponents.h"

#include "coretech/common/robot/benchmarking.h"

namespace Anki
{
  namespace Embedded
  {
    ConnectedComponents::ConnectedComponents()
      : useU16(true), componentsU16(ConnectedComponentsTemplate<u16>()), componentsS32(ConnectedComponentsTemplate<s32>())
    {
    }

    ConnectedComponents::ConnectedComponents(const s32 maxComponentSegments, const s16 maxImageWidth, MemoryStack &memory)
      : useU16(true), componentsU16(ConnectedComponentsTemplate<u16>(maxComponentSegments, maxImageWidth, memory)), componentsS32(ConnectedComponentsTemplate<s32>())
    {
    }

    ConnectedComponents::ConnectedComponents(const s32 maxComponentSegments, const s16 maxImageWidth, const bool useU16, MemoryStack &memory)
      : useU16(useU16)
    {
      if(useU16) {
        this->componentsU16 = ConnectedComponentsTemplate<u16>(maxComponentSegments, maxImageWidth, memory);
        this->componentsS32 = ConnectedComponentsTemplate<s32>();
      } else {
        this->componentsU16 = ConnectedComponentsTemplate<u16>();
        this->componentsS32 = ConnectedComponentsTemplate<s32>(maxComponentSegments, maxImageWidth, memory);
      }
    }

    Result ConnectedComponents::Extract2dComponents_FullImage(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, MemoryStack scratch)
    {
      if(this->useU16)
        return this->componentsU16.Extract2dComponents_FullImage(binaryImage, minComponentWidth, maxSkipDistance, scratch);
      else
        return this->componentsS32.Extract2dComponents_FullImage(binaryImage, minComponentWidth, maxSkipDistance, scratch);
    }

    Result ConnectedComponents::Extract2dComponents_PerRow_Initialize(MemoryStack &fastMemory, MemoryStack &slowerMemory, MemoryStack &slowestMemory)
    {
      if(this->useU16) { return this->componentsU16.Extract2dComponents_PerRow_Initialize(fastMemory, slowerMemory, slowestMemory); }
      else { return this->componentsS32.Extract2dComponents_PerRow_Initialize(fastMemory, slowerMemory, slowestMemory); }
    }

    Result ConnectedComponents::Extract2dComponents_PerRow_NextRow(const u8 * restrict binaryImageRow, const s32 imageWidth, const s16 whichRow, const s16 minComponentWidth, const s16 maxSkipDistance)
    {
      if(this->useU16) { return this->componentsU16.Extract2dComponents_PerRow_NextRow(binaryImageRow, imageWidth, whichRow, minComponentWidth, maxSkipDistance); }
      else { return this->componentsS32.Extract2dComponents_PerRow_NextRow(binaryImageRow, imageWidth, whichRow, minComponentWidth, maxSkipDistance); }
    }

    Result ConnectedComponents::Extract2dComponents_PerRow_Finalize()
    {
      if(this->useU16) { return this->componentsU16.Extract2dComponents_PerRow_Finalize(); }
      else { return this->componentsS32.Extract2dComponents_PerRow_Finalize(); }
    }

    Result ConnectedComponents::SortConnectedComponentSegments()
    {
      if(this->useU16) { return this->componentsU16.SortConnectedComponentSegments(); }
      else { return this->componentsS32.SortConnectedComponentSegments(); }
    }

    Result ConnectedComponents::SortConnectedComponentSegmentsById(MemoryStack scratch)
    {
      if(this->useU16) { return this->componentsU16.SortConnectedComponentSegmentsById(scratch); }
      else { return this->componentsS32.SortConnectedComponentSegmentsById(scratch); }
    }

    Result ConnectedComponents::CompressConnectedComponentSegmentIds(MemoryStack scratch)
    {
      if(this->useU16) { return this->componentsU16.CompressConnectedComponentSegmentIds(scratch); }
      else { return this->componentsS32.CompressConnectedComponentSegmentIds(scratch); }
    }

    Result ConnectedComponents::ComputeComponentSizes(FixedLengthList<s32> &componentSizes)
    {
      if(this->useU16) { return this->componentsU16.ComputeComponentSizes(componentSizes); }
      else { return this->componentsS32.ComputeComponentSizes(componentSizes); }
    }

    Result ConnectedComponents::ComputeComponentCentroids(FixedLengthList<Point<s16> > &componentCentroids, MemoryStack scratch)
    {
      if(this->useU16) { return this->componentsU16.ComputeComponentCentroids(componentCentroids, scratch); }
      else { return this->componentsS32.ComputeComponentCentroids(componentCentroids, scratch); }
    }

    Result ConnectedComponents::ComputeComponentBoundingBoxes(FixedLengthList<Rectangle<s16> > &componentBoundingBoxes)
    {
      if(this->useU16) { return this->componentsU16.ComputeComponentBoundingBoxes(componentBoundingBoxes); }
      else { return this->componentsS32.ComputeComponentBoundingBoxes(componentBoundingBoxes); }
    }

    Result ConnectedComponents::ComputeNumComponentSegmentsForEachId(FixedLengthList<s32> &numComponentSegments)
    {
      if(this->useU16) { return this->componentsU16.ComputeNumComponentSegmentsForEachId(numComponentSegments); }
      else { return this->componentsS32.ComputeNumComponentSegmentsForEachId(numComponentSegments); }
    }

    Result ConnectedComponents::InvalidateSmallOrLargeComponents(const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch)
    {
      if(this->useU16) { return this->componentsU16.InvalidateSmallOrLargeComponents(minimumNumPixels, maximumNumPixels, scratch); }
      else { return this->componentsS32.InvalidateSmallOrLargeComponents(minimumNumPixels, maximumNumPixels, scratch); }
    }

    Result ConnectedComponents::InvalidateSolidOrSparseComponents(const s32 sparseMultiplyThreshold, const s32 solidMultiplyThreshold, MemoryStack scratch)
    {
      if(this->useU16) { return this->componentsU16.InvalidateSolidOrSparseComponents(sparseMultiplyThreshold, solidMultiplyThreshold, scratch); }
      else { return this->componentsS32.InvalidateSolidOrSparseComponents(sparseMultiplyThreshold, solidMultiplyThreshold, scratch); }
    }

    Result ConnectedComponents::InvalidateFilledCenterComponents_shrunkRectangle(const s32 percentHorizontal, const s32 percentVertical, MemoryStack scratch)
    {
      if(this->useU16) { return this->componentsU16.InvalidateFilledCenterComponents_shrunkRectangle(percentHorizontal, percentVertical, scratch); }
      else { return this->componentsS32.InvalidateFilledCenterComponents_shrunkRectangle(percentHorizontal, percentVertical, scratch); }
    }

    Result ConnectedComponents::InvalidateFilledCenterComponents_hollowRows(const f32 minHollowRatio, MemoryStack scratch)
    {
      if(this->useU16) { return this->componentsU16.InvalidateFilledCenterComponents_hollowRows(minHollowRatio, scratch); }
      else { return this->componentsS32.InvalidateFilledCenterComponents_hollowRows(minHollowRatio, scratch); }
    }

    bool ConnectedComponents::IsValid() const
    {
      if(this->useU16) { return this->componentsU16.IsValid(); }
      else { return this->componentsS32.IsValid(); }
    }

    Result ConnectedComponents::Print() const
    {
      if(this->useU16) { return this->componentsU16.Print(); }
      else { return this->componentsS32.Print(); }
    }

    s32 ConnectedComponents::get_maximumId() const
    {
      if(this->useU16) { return this->componentsU16.get_maximumId(); }
      else { return this->componentsS32.get_maximumId(); }
    }

    s32 ConnectedComponents::get_size() const
    {
      if(this->useU16) { return this->componentsU16.get_size(); }
      else { return this->componentsS32.get_size(); }
    }

    bool ConnectedComponents::get_useU16() const
    {
      return this->useU16;
    }

    bool ConnectedComponents::get_isSortedInId() const
    {
      if(this->useU16) { return this->componentsU16.get_isSortedInId(); }
      else { return this->componentsS32.get_isSortedInId(); }
    }

    bool ConnectedComponents::get_isSortedInY() const
    {
      if(this->useU16) { return this->componentsU16.get_isSortedInY(); }
      else { return this->componentsS32.get_isSortedInY(); }
    }

    bool ConnectedComponents::get_isSortedInX() const
    {
      if(this->useU16) { return this->componentsU16.get_isSortedInX(); }
      else { return this->componentsS32.get_isSortedInX(); }
    }

    const ConnectedComponentsTemplate<u16>* ConnectedComponents::get_componentsU16() const
    {
      if(this->useU16) { return &this->componentsU16; }
      else { return NULL; }
    }

    const ConnectedComponentsTemplate<s32>* ConnectedComponents::get_componentsS32() const
    {
      if(this->useU16) { return NULL; }
      else { return &this->componentsS32; }
    }

    ConnectedComponentsTemplate<u16>* ConnectedComponents::get_componentsU16()
    {
      if(this->useU16) { return &this->componentsU16; }
      else { return NULL; }
    }

    ConnectedComponentsTemplate<s32>* ConnectedComponents::get_componentsS32()
    {
      if(this->useU16) { return NULL; }
      else { return &this->componentsS32; }
    }
  } // namespace Embedded
} // namespace Anki
