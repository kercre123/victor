#ifndef _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
#define _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_

#include "anki/embeddedVision/config.h"

#include "anki/embeddedCommon/array2d.h"
#include "anki/embeddedCommon/geometry.h"

#include "anki/embeddedVision/connectedComponents.h"

#define MAX_FIDUCIAL_MARKER_CELLS 25
#define MAX_FIDUCIAL_MARKER_CELL_PROBE_LOCATIONS 49

namespace Anki
{
  namespace Embedded
  {
    typedef enum
    {
      FIDUCIAL_CELL_UNINITIALIZED = 0,
      FIDUCIAL_CELL_NONE = 1,
      FIDUCIAL_CELL_BLOCK = 2,
      FIDUCIAL_CELL_FACE = 3,
      FIDUCIAL_CELL_ORIENTATION = 4
    } FiducialMarkerParserCellType;

    // A BlockMarker is a location Quadrilateral, with a given blockType and faceType.
    // The blockType and faceType can be computed by a FiducialMarkerParser
    class BlockMarker
    {
    public:
      Quadrilateral<s16> corners; // SQ 15.0 (Though may be changed later)
      s16 blockType, faceType;

      BlockMarker();

      void Print() const;
    }; // class BlockMarker

    class FiducialMarkerParserCell
    {
    public:
      FiducialMarkerParserCell();

      // All data from other is copied into this instance's local memory
      FiducialMarkerParserCell(const FiducialMarkerParserCell& cell2);

      // All data from probeLocations is copied into this instance's local memory
      FiducialMarkerParserCell(const FixedLengthList<Point<s16>> &probeLocations, const FixedLengthList<s16> &probeWeights, const FiducialMarkerParserCellType type);

      FiducialMarkerParserCell& operator= (const FiducialMarkerParserCell& cell2);

      const FixedLengthList<Point<s16> >& get_probeLocations() const;

      const FixedLengthList<s16>& get_probeWeights() const;

      FiducialMarkerParserCellType get_type() const;

    protected:
      FixedLengthList<Point<s16>> probeLocations; //< A list of length MAX_FIDUCIAL_MARKER_CELL_PROBE_LOCATIONS
      FixedLengthList<s16> probeWeights;
      FiducialMarkerParserCellType type;

      // The static data buffer for this object's probeLocations and probeWeights. Modifying this will change the values in probeLocations and probeWeights.
      Point<s16> probeLocationsBuffer[MAX_FIDUCIAL_MARKER_CELL_PROBE_LOCATIONS];
      Point<s16> probeWeightsBuffer[MAX_FIDUCIAL_MARKER_CELL_PROBE_LOCATIONS];

      void PrepareBuffers();
    }; // class FiducialMarkerParserCell

    class FiducialMarkerParser
    {
    public:

      // Initialize with the default grid type, converted from Matlab
      FiducialMarkerParser();

      BlockMarker ParseImage(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography);

    protected:
      FixedLengthList<FiducialMarkerParserCell> cells;

      FiducialMarkerParserCell cellsBuffer[MAX_FIDUCIAL_MARKER_CELLS];

      void PrepareBuffers();

      Result InitializeAsDefaultParser();

    private:
      // There's currently only one type of fiducial marker, so don't do these things
      FiducialMarkerParser(const FiducialMarkerParser& marker2);
      FiducialMarkerParser& operator= (const FiducialMarkerParser& marker2);
    }; // class FiducialMarkerParser
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
