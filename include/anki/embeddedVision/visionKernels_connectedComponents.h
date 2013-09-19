#ifndef _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
#define _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_

#include "anki/embeddedVision/config.h"
#include "anki/embeddedCommon.h"

namespace Anki
{
  namespace Embedded
  {
    Result Extract2dComponents(const Array<u8> &binaryImage, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment> &extractedComponents, MemoryStack scratch);
    Result Extract1dComponents(const u8 * restrict binaryImageRow, const s16 binaryImageWidth, const s16 minComponentWidth, const s16 maxSkipDistance, FixedLengthList<ConnectedComponentSegment> &extractedComponents);

    // Sort the components by id, y, then xStart
    // TODO: determine how fast this method is, then suggest usage
    Result SortConnectedComponentSegments(FixedLengthList<ConnectedComponentSegment> &components);

    // The list of components may have unused ids. This function compresses the set of ids, so that
    // max(ids) == numberOfUniqueValues(ids). For example, the list of ids {0,4,5,7} would be
    // changed to {0,1,2,3}.
    //
    // For a components parameter that has a maximum id of N, this function requires
    // 3n + 1 bytes of scratch.
    //
    // TODO: If scratch usage is a bigger issue than computation time, this could be done with a bitmask
    u16 CompressConnectedComponentSegmentIds(FixedLengthList<ConnectedComponentSegment> &components, MemoryStack scratch);

    // Iterate through components, and return the maximum id
    u16 FindMaximumId(FixedLengthList<ConnectedComponentSegment> &components);

    // Goes through the list components, and computes the number of pixels for each.
    // For any componentId with less than minimumNumPixels pixels, all ConnectedComponentSegment with that id will have their ids set to zero
    //
    // For a components parameter that has a maximum id of N, this function requires
    // 4n + 4 bytes of scratch.
    Result MarkSmallOrLargeComponentsAsInvalid(FixedLengthList<ConnectedComponentSegment> &components, const s32 minimumNumPixels, const s32 maximumNumPixels, MemoryStack scratch);

    // Returns a positive s64 if a > b, a negative s64 is a < b, or zero if they are identical
    // TODO: Doublecheck that this is correct for corner cases
    inline s64 CompareConnectedComponentSegments(const ConnectedComponentSegment &a, const ConnectedComponentSegment &b)
    {
      const s64 idDifference = static_cast<s64>(u16_MAX) * static_cast<s64>(u16_MAX) * (static_cast<s64>(a.id) - static_cast<s64>(b.id));
      const s64 yDifference = static_cast<s64>(u16_MAX) * (static_cast<s64>(a.y) - static_cast<s64>(b.y));
      const s64 xStartDifference = (static_cast<s64>(a.xStart) - static_cast<s64>(b.xStart));

      return idDifference + yDifference + xStartDifference;
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_VISIONKERNELS_CONNECTEDCOMPONENTS_H_
