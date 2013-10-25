#ifndef _ANKICORETECHEMBEDDED_VISION_FIXED_LENGTH_LIST_VISION_H_
#define _ANKICORETECHEMBEDDED_VISION_FIXED_LENGTH_LIST_VISION_H_

#include "anki/vision/robot/connectedComponents.h"

#include "anki/common/robot/fixedLengthList.h"

namespace Anki
{
  namespace Embedded
  {
    template<> Result FixedLengthList<ConnectedComponentSegment>::Print(const char * const variableName, const s32 minIndex, const s32 maxIndex) const;
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_FIXED_LENGTH_LIST_VISION_H_
