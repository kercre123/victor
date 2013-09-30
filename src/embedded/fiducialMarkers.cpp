#include "anki/embeddedVision/fiducialMarkers.h"

#include "anki/embeddedVision/fixedLengthList_vision.h"
#include "anki/embeddedVision/miscVisionKernels.h"
#include "anki/embeddedVision/draw_vision.h"

#include "fiducialMarkerDefinitionType0.h"

#define INITIALIZE_WITH_DEFINITION_TYPE 0

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
        this->probeWeights[i] = bit2.probeWeights[i];
      }
    } // FiducialMarkerParserBit::FiducialMarkerParserBit(const FiducialMarkerParserBit& bit2)

    FiducialMarkerParserBit::FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probesWeights, const s32 numProbes, const FiducialMarkerParserBitType type)
    {
      PrepareBuffers();

      assert(numProbes <= MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS);

      this->probeLocations.set_size(numProbes);
      this->probeWeights.set_size(numProbes);
      this->type = type;

      for(s32 i=0; i<probeLocations.get_size(); i++) {
        this->probeLocations[i].x = probesX[i];
        this->probeLocations[i].y = probesY[i];
        this->probeWeights[i] = probeWeights[i];
      }
    } // FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probesWeights, const s32 numProbes)

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
    } // FiducialMarkerParserBit& FiducialMarkerParserBit::operator= (const FiducialMarkerParserBit& bit2)

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

      this->probeLocations = FixedLengthList<Point<s16> >(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, &probeLocationsBuffer[0], NUM_BYTES_probeLocationsBuffer);
      this->probeWeights = FixedLengthList<s16>(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, &probeWeightsBuffer[0], NUM_BYTES_probeWeightsBuffer);
    }

    // Initialize with the default grid type, converted from Matlab
    FiducialMarkerParser::FiducialMarkerParser()
    {
      PrepareBuffers();
      InitializeAsDefaultParser();
    }

    FiducialMarkerParser::FiducialMarkerParser(const FiducialMarkerParser& marker2)
    {
      PrepareBuffers();

      memcpy(this->bitsBuffer, marker2.bitsBuffer, NUM_BYTES_bitsBuffer);

      this->bits.set_size(marker2.bits.get_size());
    }

    BlockMarker FiducialMarkerParser::ParseImage(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography)
    {
      BlockMarker marker;

      // TODO: implement

      return marker;
    }

    FiducialMarkerParser& FiducialMarkerParser::operator= (const FiducialMarkerParser& marker2)
    {
      PrepareBuffers();

      memcpy(this->bitsBuffer, marker2.bitsBuffer, NUM_BYTES_bitsBuffer);

      this->bits.set_size(marker2.bits.get_size());

      return *this;
    }

    void FiducialMarkerParser::PrepareBuffers()
    {
      this->bits = FixedLengthList<FiducialMarkerParserBit>(MAX_FIDUCIAL_MARKER_BITS, &bitsBuffer[0], NUM_BYTES_bitsBuffer);
      this->bits.IsValid();
    }

    Result FiducialMarkerParser::InitializeAsDefaultParser()
    {
      if(INITIALIZE_WITH_DEFINITION_TYPE == 0) {
        assert(NUM_BITS_TYPE_0 <= MAX_FIDUCIAL_MARKER_BITS);
        assert(NUM_PROBES_PER_BIT_TYPE_0 <= MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS);

        this->bits.Clear();

        for(s32 i=0; i<NUM_BITS_TYPE_0; i++) {
          this->bits.PushBack(FiducialMarkerParserBit(probesX_type0[i], probesY_type0[i], probeWeights_type0[i], NUM_PROBES_PER_BIT_TYPE_0, bitTypes_type0[i]));
        }
      } // if(INITIALIZE_WITH_DEFINITION_TYPE == 0)
      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki