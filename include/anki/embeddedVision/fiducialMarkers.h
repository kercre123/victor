#ifndef _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
#define _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_

#include "anki/embeddedVision/config.h"

#include "anki/embeddedCommon/array2d.h"
#include "anki/embeddedCommon/geometry.h"

#include "anki/embeddedVision/connectedComponents.h"

#define MAX_FIDUCIAL_MARKER_BITS 25
#define MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS 81

#define NUM_BYTES_probeLocationsBuffer (sizeof(Point<s16>)*MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS + MEMORY_ALIGNMENT + 32)
#define NUM_BYTES_probeWeightsBuffer (sizeof(s16)*MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS  + MEMORY_ALIGNMENT + 32)
#define NUM_BYTES_bitsBuffer (sizeof(FiducialMarkerParserBit)*MAX_FIDUCIAL_MARKER_BITS + MEMORY_ALIGNMENT + 32)

namespace Anki
{
  namespace Embedded
  {
    // A BlockMarker is a location Quadrilateral, with a given blockType and faceType.
    // The blockType and faceType can be computed by a FiducialMarkerParser
    class BlockMarker
    {
    public:
      typedef enum
      {
        ORIENTATION_UNKNOWN = 0,
        ORIENTATION_UP = 1,
        ORIENTATION_DOWN = 2,
        ORIENTATION_LEFT = 3,
        ORIENTATION_RIGHT = 4,
      } Orientation;

      Quadrilateral<s16> corners; // SQ 15.0 (Though may be changed later)
      s16 blockType, faceType;
      Orientation orientation;

      BlockMarker();

      void Print() const;
    }; // class BlockMarker

    class FiducialMarkerParserBit
    {
    public:
      typedef enum
      {
        FIDUCIAL_BIT_UNINITIALIZED = 0,
        FIDUCIAL_BIT_NONE = 1,
        FIDUCIAL_BIT_BLOCK = 2,
        FIDUCIAL_BIT_FACE = 3,
        FIDUCIAL_BIT_ORIENTATION_LEFT = 4,
        FIDUCIAL_BIT_ORIENTATION_RIGHT = 5,
        FIDUCIAL_BIT_ORIENTATION_UP = 6,
        FIDUCIAL_BIT_ORIENTATION_DOWN = 7,
        FIDUCIAL_BIT_CHECKSUM = 8
      } Type;

      FiducialMarkerParserBit();

      // All data from other is copied into this instance's local memory
      FiducialMarkerParserBit(const FiducialMarkerParserBit& bit2);

      // All data from probeLocations is copied into this instance's local memory
      FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probeWeights, const s32 numProbes, const FiducialMarkerParserBit::Type type, const s32 numFractionalBits);

      Result ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, s16 &meanValue) const;

      FiducialMarkerParserBit& operator= (const FiducialMarkerParserBit& bit2);

      const FixedLengthList<Point<s16> >& get_probeLocations() const;

      const FixedLengthList<s16>& get_probeWeights() const;

      FiducialMarkerParserBit::Type get_type() const;

      s32 get_numFractionalBits() const;

    protected:
      FixedLengthList<Point<s16>> probeLocations; //< A list of length MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS
      FixedLengthList<s16> probeWeights;
      FiducialMarkerParserBit::Type type;
      s32 numFractionalBits;

      // The static data buffer for this object's probeLocations and probeWeights. Modifying this will change the values in probeLocations and probeWeights.
      //Point<s16> probeLocationsBuffer[MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS];
      //s16 probeWeightsBuffer[MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS];
      char probeLocationsBuffer[NUM_BYTES_probeLocationsBuffer];
      char probeWeightsBuffer[NUM_BYTES_probeWeightsBuffer];

      void PrepareBuffers();
    }; // class FiducialMarkerParserBit

    class FiducialMarkerParser
    {
    public:

      // Initialize with the default grid type, converted from Matlab
      FiducialMarkerParser();

      FiducialMarkerParser(const FiducialMarkerParser& marker2);

      Result ExtractBlockMarker(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, const f32 minContrastRatio, BlockMarker &marker, MemoryStack scratch) const;

      FiducialMarkerParser& operator= (const FiducialMarkerParser& marker2);

    protected:
      FixedLengthList<FiducialMarkerParserBit> bits;

      // The indexes of the four orientation bits
      s32 upBitIndex;
      s32 downBitIndex;
      s32 leftBitIndex;
      s32 rightBitIndex;

      //FiducialMarkerParserBit bitsBuffer[MAX_FIDUCIAL_MARKER_BITS];
      char bitsBuffer[NUM_BYTES_bitsBuffer];

      static bool IsChecksumValid(const FixedLengthList<u8> &checksumBits, const FixedLengthList<u8> &blockBits, const FixedLengthList<u8> &faceBits);

      void PrepareBuffers();

      Result InitializeAsDefaultParser();

      Result DetermineOrientationAndBinarizeAndReorderCorners(const FixedLengthList<s16> &meanValues, const f32 minContrastRatio, BlockMarker &marker, FixedLengthList<u8> &binarizedBits, MemoryStack scratch) const;

      Result DecodeId(const FixedLengthList<u8> &binarizedBits, s16 &blockType, s16 &faceType, MemoryStack scratch) const;

      // Starting at startIndex, search through this->bits to find the first instance of the given type
      // Returns -1 if the type wasn't found
      s32 FindFirstBitOfType(const FiducialMarkerParserBit::Type type, const s32 startIndex) const;
    }; // class FiducialMarkerParser
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
