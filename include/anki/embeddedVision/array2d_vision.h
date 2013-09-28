#ifndef _ANKICORETECHEMBEDDED_VISION_ARRAY2D_VISION_H_
#define _ANKICORETECHEMBEDDED_VISION_ARRAY2D_VISION_H_

#include "anki/embeddedVision/config.h"
#include "anki/embeddedCommon.h"
#include "anki/embeddedVision/connectedComponents.h"
#include "anki/embeddedVision/fiducialMarkers.h"

#include <stdio.h>
#include <stdlib.h>


namespace Anki
{
  namespace Embedded
  {
    template<> Result Array<ConnectedComponentSegment>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
    template<> Result Array<BlockMarker>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const;
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_ARRAY2D_VISION_H_
