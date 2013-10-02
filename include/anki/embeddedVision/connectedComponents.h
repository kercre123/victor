#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_

#include "anki/embeddedVision/config.h"
#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    // A 1d, run-length encoded piece of a 2d component
    class ConnectedComponentSegment
    {
    public:
      s16 xStart, xEnd, y;
      u16 id;

      ConnectedComponentSegment();

      ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y = -1, const u16 id = 0);

      void Print() const;

      bool operator== (const ConnectedComponentSegment &component2) const;
    }; // class ConnectedComponentSegment

    class ConnectedComponents
    {
    public:
      // Returns a positive s64 if a > b, a negative s64 is a < b, or zero if they are identical
      // The ordering of components is first by id (the ids are sorted in increasing value, but with zero at the end {1...MAX_VALUE,0}), then y, then xStart
      // TODO: Doublecheck that this is correct for corner cases
      static inline s64 CompareConnectedComponentSegments(const ConnectedComponentSegment &a, const ConnectedComponentSegment &b);

      // TODO: when this class is converted to accept single scanlines as input, this will be significantly refactored
      static Result Extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment> &extractedComponents);

      ConnectedComponents();

      // Constructor for a ConnectedComponents, pointing to user-allocated MemoryStack
      ConnectedComponents(const s32 maxComponentSegments, MemoryStack &memory);

      // TODO: when this class is converted to accept single scanlines as input, this will be significantly refactored
      Result Extract2dComponents(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, MemoryStack scratch);

      // Sort the components by id (the ids are sorted in increasing value, but with zero at the end {1...MAX_VALUE,0}), then y, then xStart
      // WARNING: This method is really slow if called first. If you have the memory available, call SortConnectedComponentSegmentsById() first.
      Result SortConnectedComponentSegments();

      // Sort the components by id
      // Requires numValidComponentSegments*sizeof(ConnectedComponentSegment) bytes of scratch
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
      // Note: this is probably inefficient, compared with interlacing the loops in a kernel
      Result ComputeComponentSizes(FixedLengthList<s32> &componentSizes);

      // Iterate through components, and compute the centroid of each component componentCentroids
      // must be at least sizeof(Point<s16>)*(maximumdId+1) bytes
      // Note: this is probably inefficient, compared with interlacing the loops in a kernel
      //
      // For a ConnectedComponent that has a maximum id of N, this function requires
      // 4n + 4 bytes of scratch.
      Result ComputeComponentCentroids(FixedLengthList<Point<s16> > &componentCentroids, MemoryStack scratch);

      // Iterate through components, and compute bounding box for each component
      // componentBoundingBoxes must be at least sizeof(Rectangle<s16>)*(maximumdId+1) bytes
      // Note: this is probably inefficient, compared with interlacing the loops in a kernel
      Result ComputeComponentBoundingBoxes(FixedLengthList<Rectangle<s16> > &componentBoundingBoxes);

      // Iterate through components, and compute the number of componentSegments that have each id
      // componentSizes must be at least sizeof(s32)*(maximumdId+1) bytes
      // Note: this is probably inefficient, compared with interlacing the loops in a kernel
      Result ComputeNumComponentSegmentsForEachId(FixedLengthList<s32> &numComponentSegments);

      // Goes through the list components, and computes the number of pixels for each.
      // For any componentId with less than minimumNumPixels pixels, all ConnectedComponentSegment with that id will have their ids set to zero
      //
      // For a ConnectedComponent that has a maximum id of N, this function requires
      // 4n + 4 bytes of scratch.
      Result InvalidateSmallOrLargeComponents(const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch);

      // Goes through the list components, and computes the "solidness", which is the ratio of
      // "numPixels / (boundingWidth*boundingHeight)". For any componentId with that is too solid or
      // sparse (opposite of solid), all ConnectedComponentSegment with that id will have their ids
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
      // Note: This can overflow if the number of pixels is greater than 2^26 (a bit more Ultra-HD
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
      Result InvalidateFilledCenterComponents(const s32 percentHorizontal, const s32 percentVertical, MemoryStack scratch);

      Result PushBack(const ConnectedComponentSegment &value);

      // Note that this is a const-only accessor function. The ConnectedComponets class keeps a lot
      // of tabs on sorting and maximumId and such, so no one else should be directly modifying the
      // buffers.
      inline const ConnectedComponentSegment* Pointer(const s32 index) const;
      inline const ConnectedComponentSegment& operator[](const s32 index) const;

      bool IsValid() const;

      u16 get_maximumId() const;

      s32 get_size() const;

      bool get_isSortedInId() const;
      bool get_isSortedInY() const;
      bool get_isSortedInX() const;

    protected:
      FixedLengthList<ConnectedComponentSegment> components;

      bool isSortedInId;
      bool isSortedInY;
      bool isSortedInX;

      u16 maximumId;

      // Iterate through components, and update the maximum id
      Result FindMaximumId();
    }; // class ConnectedComponents

#pragma mark --- Implementations ---

    inline s64 ConnectedComponents::CompareConnectedComponentSegments(const ConnectedComponentSegment &a, const ConnectedComponentSegment &b)
    {
      // Wraps zero around to MAX_u16
      u16 idA = a.id;
      u16 idB = b.id;

      idA -= 1;
      idB -= 1;

      const s64 idDifference = static_cast<s64>(u16_MAX) * static_cast<s64>(u16_MAX) * (static_cast<s64>(idA) - static_cast<s64>(idB));
      const s64 yDifference = static_cast<s64>(u16_MAX) * (static_cast<s64>(a.y) - static_cast<s64>(b.y));
      const s64 xStartDifference = (static_cast<s64>(a.xStart) - static_cast<s64>(b.xStart));

      return idDifference + yDifference + xStartDifference;
    }

    inline const ConnectedComponentSegment* ConnectedComponents::Pointer(const s32 index) const
    {
      return components.Pointer(index);
    }

    inline const ConnectedComponentSegment& ConnectedComponents::operator[](const s32 index) const
    {
      return *components.Pointer(index);
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
