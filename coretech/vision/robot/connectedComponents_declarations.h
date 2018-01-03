/**
File: connectedComponents_declarations.h
Author: Peter Barnum
Created: 2013

Compute the connected components from a binary image.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_DECLARATIONS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_DECLARATIONS_H_

#include "coretech/common/robot/config.h"
#include "coretech/common/shared/types.h"
#include "coretech/common/robot/memory.h"
#include "coretech/common/robot/fixedLengthList_declarations.h"

namespace Anki
{
  namespace Embedded
  {
    // A 1d, run-length encoded piece of a 2d component
    // The type is for storing the ID. u16 is enough for QVGA, but VGA and above need s32
    template<typename Type> class ConnectedComponentSegment
    {
    public:
      // xStart, xEnd, y use array indexes, meaning the first pixel is at (0,0), not (0.5,0.5) like true coordinates
      s16 xStart, xEnd, y;
      Type id;

      ConnectedComponentSegment();

      ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y = -1, const Type id = 0);

      // Returns a positive s64 if a > b, a negative s64 is a < b, or zero if they are identical
      // The ordering of components is first by id (the ids are sorted in increasing value, but with zero at the end {1...MAX_VALUE,0}), then y, then xStart
      // TODO: Doublecheck that this is correct for corner cases
      static inline s64 Compare(const ConnectedComponentSegment<Type> &a, const ConnectedComponentSegment<Type> &b);

      void Print() const;

      bool operator== (const ConnectedComponentSegment &component2) const;
    }; // class ConnectedComponentSegment

    // Template for ConnectedComponents. See ConnectedComponents for documentation.
    template<typename Type> class ConnectedComponentsTemplate
    {
    public:

      static Result Extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment<Type> > &extractedComponents);

      ConnectedComponentsTemplate();

      ConnectedComponentsTemplate(const Type maxComponentSegments, const s16 maxImageWidth, MemoryStack &memory);

      Result Extract2dComponents_FullImage(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, MemoryStack scratch);

      Result Extract2dComponents_PerRow_Initialize(MemoryStack &fastMemory, MemoryStack &slowerMemory, MemoryStack &slowestMemory);
      Result Extract2dComponents_PerRow_NextRow(const u8 * restrict binaryImageRow, const s32 imageWidth, const s16 whichRow, const s16 minComponentWidth, const s16 maxSkipDistance);
      Result Extract2dComponents_PerRow_Finalize();

      Result SortConnectedComponentSegments();

      Result SortConnectedComponentSegmentsById(MemoryStack scratch);

      Result CompressConnectedComponentSegmentIds(MemoryStack scratch);

      Result ComputeComponentSizes(FixedLengthList<s32> &componentSizes);

      Result ComputeComponentCentroids(FixedLengthList<Point<s16> > &componentCentroids, MemoryStack scratch);

      Result ComputeComponentBoundingBoxes(FixedLengthList<Rectangle<s16> > &componentBoundingBoxes);

      Result ComputeNumComponentSegmentsForEachId(FixedLengthList<s32> &numComponentSegments);

      Result InvalidateSmallOrLargeComponents(const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch);

      Result InvalidateSolidOrSparseComponents(const s32 sparseMultiplyThreshold, const s32 solidMultiplyThreshold, MemoryStack scratch);

      Result InvalidateFilledCenterComponents_shrunkRectangle(const s32 percentHorizontal, const s32 percentVertical, MemoryStack scratch);

      Result InvalidateFilledCenterComponents_hollowRows(const f32 minHollowRatio, MemoryStack scratch);

      Result PushBack(const ConnectedComponentSegment<Type> &value);

      // Note that this is a const-only accessor function. The ConnectedComponets class keeps a lot
      // of tabs on sorting and maximumId and such, so no one else should be directly modifying the
      // buffers.
      inline const ConnectedComponentSegment<Type>* Pointer(const s32 index) const;
      inline const ConnectedComponentSegment<Type>& operator[](const s32 index) const;

      bool IsValid() const;

      Result Print() const;

      Type get_maximumId() const;

      s32 get_size() const;

      bool get_isSortedInId() const;
      bool get_isSortedInY() const;
      bool get_isSortedInX() const;

    protected:
      enum State
      {
        STATE_INVALID,
        STATE_CONSTRUCTED,
        STATE_INITIALIZED,
        STATE_FINALIZED
      };

      FixedLengthList<ConnectedComponentSegment<Type> > components;
      FixedLengthList<ConnectedComponentSegment<Type> > currentComponents1d;
      FixedLengthList<ConnectedComponentSegment<Type> > previousComponents1d;
      FixedLengthList<ConnectedComponentSegment<Type> > newPreviousComponents1d;
      FixedLengthList<Type> equivalentComponents;

      State curState;

      bool isSortedInId;
      bool isSortedInY;
      bool isSortedInX;

      Type maximumId;
      s32 maxImageWidth;
      s32 maxComponentSegments;

      // Iterate through components, and update the maximum id
      Result FindMaximumId();
    }; // class ConnectedComponentsTemplate

    // A ConnectedComponents class holds a list of ConnectedComponentSegment<Type> objects
    // It can incrementally parse an input binary image per-row, updating its global list as it goes
    // It also contains various utilities to remove poor-quality components
    class ConnectedComponents
    {
    public:

      ConnectedComponents();

      // Constructor for a ConnectedComponents, pointing to user-allocated MemoryStack
      // The memory should remain valid for the entire life of the object
      ConnectedComponents(const s32 maxComponentSegments, const s16 maxImageWidth, MemoryStack &memory); //< This default constructor creates a u16 object
      ConnectedComponents(const s32 maxComponentSegments, const s16 maxImageWidth, const bool useU16, MemoryStack &memory);

      // Extract 2d connected components from binaryImage All extracted components are stored in a
      // single list of ComponentSegments
      Result Extract2dComponents_FullImage(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, MemoryStack scratch);

      // Methods to parse an input binary image per-row, updating this object's global list as it goes
      //
      // WARNING:
      // The memory allocated in Extract2dComponents_PerRow_Initialize() must be valid until
      // Extract2dComponents_PerRow_Finalize() is called. It does not have to be in the same
      // location as the memory used by the constructor
      // Note: fastMemory and slowMemory can be the same object pointing to the same memory
      Result Extract2dComponents_PerRow_Initialize(MemoryStack &fastMemory, MemoryStack &slowerMemory, MemoryStack &slowestMemory);
      Result Extract2dComponents_PerRow_NextRow(const u8 * restrict binaryImageRow, const s32 imageWidth, const s16 whichRow, const s16 minComponentWidth, const s16 maxSkipDistance);
      Result Extract2dComponents_PerRow_Finalize();

      // Sort the components by id (the ids are sorted in increasing value, but with zero at the end {1...MAX_VALUE,0}), then y, then xStart
      // WARNING: This method is really slow if called first. If you have the memory available, call SortConnectedComponentSegmentsById() first.
      Result SortConnectedComponentSegments();

      // Sort the components by id. This will retain the original ordering as well, so if the
      // components are already sorted in y, the output of this method will be sorted in id and y.
      // Requires numValidComponentSegments*sizeof(ConnectedComponentSegment<u16>) bytes of scratch
      Result SortConnectedComponentSegmentsById(MemoryStack scratch);

      // The list of components may have unused ids. This function compresses the set of ids, so that
      // max(ids) == numberOfUniqueValues(ids). For example, the list of ids {0,4,5,7} would be
      // changed to {0,1,2,3}.
      //
      // For a ConnectedComponent that has a maximum id of N, this function requires
      // 3n + 1 bytes of scratch.
      //
      // TODO: If scratch usage is a bigger issue than computation time, this could be done with a bitmask
      Result CompressConnectedComponentSegmentIds(MemoryStack scratch);

      // Iterate through components, and compute the number of pixels for each component
      // componentSizes must be at least sizeof(s32)*(maximumdId+1) bytes
      // NOTE: this is probably inefficient, compared with interlacing the loops in a kernel
      Result ComputeComponentSizes(FixedLengthList<s32> &componentSizes);

      // Iterate through components, and compute the centroid of each component componentCentroids
      // must be at least sizeof(Point<s16>)*(maximumdId+1) bytes
      // NOTE: this is probably inefficient, compared with interlacing the loops in a kernel
      //
      // For a ConnectedComponent that has a maximum id of N, this function requires
      // 4n + 4 bytes of scratch.
      Result ComputeComponentCentroids(FixedLengthList<Point<s16> > &componentCentroids, MemoryStack scratch);

      // Iterate through components, and compute bounding box for each component
      // componentBoundingBoxes must be at least sizeof(Rectangle<s16>)*(maximumdId+1) bytes
      // NOTE: this is probably inefficient, compared with interlacing the loops in a kernel
      Result ComputeComponentBoundingBoxes(FixedLengthList<Rectangle<s16> > &componentBoundingBoxes);

      // Iterate through components, and compute the number of componentSegments that have each id
      // componentSizes must be at least sizeof(s32)*(maximumdId+1) bytes
      // NOTE: this is probably inefficient, compared with interlacing the loops in a kernel
      Result ComputeNumComponentSegmentsForEachId(FixedLengthList<s32> &numComponentSegments);

      // Goes through the list components, and computes the number of pixels for each.
      // For any componentId with less than minimumNumPixels pixels, all ConnectedComponentSegment<u16> with that id will have their ids set to zero
      //
      // For a ConnectedComponent that has a maximum id of N, this function requires
      // 4n + 4 bytes of scratch.
      Result InvalidateSmallOrLargeComponents(const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch);

      // Goes through the list components, and computes the "solidness", which is the ratio of
      // "numPixels / (boundingWidth*boundingHeight)". For any componentId with that is too solid or
      // sparse (opposite of solid), all ConnectedComponentSegment<u16> with that id will have their ids
      // set to zero
      //
      // The SQ26.5 parameter sparseMultiplyThreshold is set so that a component is invalid if
      // "sparseMultiplyThreshold*numPixels < boundingWidth*boundingHeight". A resonable value is
      // between 5<<5 = 160 and 100<<5 = 3200.
      //
      // The SQ26.5 parameter solidMultiplyThreshold is set so that a component is invalid if
      // "solidMultiplyThreshold*numPixels > boundingWidth*boundingHeight". A resonable value is
      // between 1.5*pow(2,5) = 48 and 5<<5 = 160.
      //
      // NOTE: This can overflow if the number of pixels is greater than 2^26 (a bit more Ultra-HD
      //       resolution)
      //
      // For a ConnectedComponent that has a maximum id of N, this function requires 8N + 8 bytes
      // of scratch.
      Result InvalidateSolidOrSparseComponents(const s32 sparseMultiplyThreshold, const s32 solidMultiplyThreshold, MemoryStack scratch);

      // If a component doesn't have a hollow center, it's not a fiducial. Based on a component's
      // centroid, and its maximum extent, this method makes sure no componentSegment is inside of
      // an inner rectangle. For example, take a component centered at (50,50), that is 20 pixels
      // wide and high. If percentHorizontal=0.5 and percentVertical=0.25, then no componentSegment
      // should intersect the rectangle between (40,45) and (60,55).
      //
      // percentHorizontal and percentVertical are SQ23.8,
      // and should range from (0.0, 1.0), non-inclusive
      //
      // For a ConnectedComponent that has a maximum id of N, this function requires 10N + 10 bytes
      // of scratch.
      Result InvalidateFilledCenterComponents_shrunkRectangle(const s32 percentHorizontal, const s32 percentVertical, MemoryStack scratch);

      // Go along each row of components. Find the maximum difference between the end of one
      // component and the start of the next. The amount of space in the center of a component is
      // approximated as the sum of the per-row max distances, divided by the number of filled pixels.
      //
      // For example:
      // If minHollowRatio==0.5, this means that a component must have at least half as many interior as exterior pixels.
      // If minHollowRatio==1.0, this means that a component must have at least an equal number of interior and exterior pixels.
      // If minHollowRatio==2.0, this means that a component must have at least twice as many interior as exterior pixels.
      // TODO: what is a reasonable value? 1.0?
      Result InvalidateFilledCenterComponents_hollowRows(const f32 minHollowRatio, MemoryStack scratch);

      bool IsValid() const;

      Result Print() const;

      s32 get_maximumId() const;

      s32 get_size() const;

      bool get_useU16() const;

      bool get_isSortedInId() const;
      bool get_isSortedInY() const;
      bool get_isSortedInX() const;

      const ConnectedComponentsTemplate<u16>* get_componentsU16() const;
      const ConnectedComponentsTemplate<s32>* get_componentsS32() const;

      ConnectedComponentsTemplate<u16>* get_componentsU16();
      ConnectedComponentsTemplate<s32>* get_componentsS32();

    protected:
      // Only one of these classes will be initialized, based on the constructor
      bool useU16;
      ConnectedComponentsTemplate<u16> componentsU16;
      ConnectedComponentsTemplate<s32> componentsS32;
    }; // class ConnectedComponents
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_DECLARATIONS_H_
