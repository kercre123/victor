#include "anki/embeddedVision/fixedLengthList_vision.h"
#include "anki/embeddedVision/array2d_vision.h"

namespace Anki
{
  namespace Embedded
  {
    template<> Result FixedLengthList<ConnectedComponentSegment>::Print(const char * const variableName, const s32 minIndex, const s32 maxIndex) const
    {
      return Array<ConnectedComponentSegment>::Print(variableName, 0, 0, MAX(0,minIndex), MIN(maxIndex, this->get_size()-1));
    } // template<> Result Array<ConnectedComponentSegment >::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
  } // namespace Embedded
} // namespace Anki