#include "anki/embeddedVision/fiducialMarkers.h"

#include "anki/embeddedVision/fixedLengthList_vision.h"
#include "anki/embeddedVision/miscVisionKernels.h"
#include "anki/embeddedVision/draw_vision.h"

#include "fiducialMarkerDefinitionType0.h"

namespace Anki
{
  namespace Embedded
  {
    BlockMarker::BlockMarker()
    {
    } // BlockMarker::BlockMarker()

    void BlockMarker::Print() const
    {
      printf("[%d,%d: (%d,%d) (%d,%d) (%d,%d) (%d,%d)] ", blockType, faceType, corners[0].x, corners[0].y, corners[1].x, corners[1].y, corners[2].x, corners[2].y, corners[3].x, corners[3].y);
    }

    FiducialMarkerParserBit::FiducialMarkerParserBit()
    {
      PrepareBuffers();
    }

    // All data from other is copied into this instance's local memory
    FiducialMarkerParserBit::FiducialMarkerParserBit(const FiducialMarkerParserBit& bit2)
    {
      PrepareBuffers();

      assert(bit2.probeLocations.get_size() == bit2.probeWeights.get_size());

      this->probeLocations.set_size(bit2.probeLocations.get_size());
      this->probeWeights.set_size(bit2.probeWeights.get_size());
      this->type = bit2.type;

      for(s32 i=0; i<bit2.probeLocations.get_size(); i++) {
        this->probeLocations[i] = bit2.probeLocations[i];
        this->probeLocations[i] = bit2.probeLocations[i];
      }
    }

    // All data from probeLocations is copied into this instance's local memory
    FiducialMarkerParserBit::FiducialMarkerParserBit(const FixedLengthList<Point<s16>> &probeLocations, const FixedLengthList<s16> &probeWeights, const FiducialMarkerParserBitType type)
    {
      PrepareBuffers();

      assert(probeLocations.get_size() == probeWeights.get_size());

      this->probeLocations.set_size(probeLocations.get_size());
      this->probeWeights.set_size(probeWeights.get_size());
      this->type = type;

      for(s32 i=0; i<probeLocations.get_size(); i++) {
        this->probeLocations[i] = probeLocations[i];
        this->probeLocations[i] = probeLocations[i];
      }
    }

    FiducialMarkerParserBit& FiducialMarkerParserBit::operator= (const FiducialMarkerParserBit& bit2)
    {
      PrepareBuffers();

      assert(bit2.probeLocations.get_size() == bit2.probeWeights.get_size());

      this->probeLocations.set_size(bit2.probeLocations.get_size());
      this->probeWeights.set_size(bit2.probeWeights.get_size());
      this->type = bit2.type;

      for(s32 i=0; i<bit2.probeLocations.get_size(); i++) {
        this->probeLocations[i] = bit2.probeLocations[i];
        this->probeLocations[i] = bit2.probeLocations[i];
      }

      return *this;
    }

    const FixedLengthList<Point<s16>>& FiducialMarkerParserBit::get_probeLocations() const
    {
      return this->probeLocations;
    }

    const FixedLengthList<s16>& FiducialMarkerParserBit::get_probeWeights() const
    {
      return this->probeWeights;
    }

    FiducialMarkerParserBitType FiducialMarkerParserBit::get_type() const
    {
      return this->type;
    }

    void FiducialMarkerParserBit::PrepareBuffers()
    {
      this->type = FIDUCIAL_BIT_UNINITIALIZED;

      this->probeLocations = FixedLengthList<Point<s16> >(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, &probeLocationsBuffer[0], sizeof(probeLocationsBuffer[0])*MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS);
      this->probeWeights = FixedLengthList<s16>(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, &probeWeightsBuffer[0], sizeof(probeWeightsBuffer[0])*MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS);
    }

    // Initialize with the default grid type, converted from Matlab
    FiducialMarkerParser::FiducialMarkerParser()
    {
      PrepareBuffers();
      InitializeAsDefaultParser();
    }

    BlockMarker FiducialMarkerParser::ParseImage(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography)
    {
      BlockMarker marker;

      // TODO: implement

      return marker;
    }

    void FiducialMarkerParser::PrepareBuffers()
    {
      this->bits = FixedLengthList<FiducialMarkerParserBit>(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, &bitsBuffer[0], sizeof(bitsBuffer[0])*MAX_FIDUCIAL_MARKER_BITS);
    }

    Result FiducialMarkerParser::InitializeAsDefaultParser()
    {
      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki