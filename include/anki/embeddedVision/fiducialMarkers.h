#ifndef _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
#define _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_

#include "anki/embeddedVision/config.h"

#include "anki/embeddedCommon/array2d.h"
#include "anki/embeddedCommon/geometry.h"

#include "anki/embeddedVision/connectedComponents.h"

namespace Anki
{
  namespace Embedded
  {
    // A BlockMarker is a location Quadrilateral, with a given blockType and faceType.
    // The blockType and faceType can be computed by a FiducialMarkerParser
    class BlockMarker
    {
    public:
      Quadrilateral<s16> corners; // SQ 15.0 (Though may be changed later)
      //Point<s16> corners[4];
      // Array<f64> homography;
      s16 blockType, faceType;

      BlockMarker();

      void Print() const;
    }; // class BlockMarker
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
