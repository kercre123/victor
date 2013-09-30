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
      this->numFractionalBits = bit2.numFractionalBits;

      for(s32 i=0; i<bit2.probeLocations.get_size(); i++) {
        this->probeLocations[i] = bit2.probeLocations[i];
        this->probeWeights[i] = bit2.probeWeights[i];
      }
    } // FiducialMarkerParserBit::FiducialMarkerParserBit(const FiducialMarkerParserBit& bit2)

    FiducialMarkerParserBit::FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probeWeights, const s32 numProbes, const FiducialMarkerParserBitType type, const s32 numFractionalBits)
    {
      PrepareBuffers();

      assert(numProbes <= MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS);

      this->probeLocations.set_size(numProbes);
      this->probeWeights.set_size(numProbes);
      this->type = type;
      this->numFractionalBits = numFractionalBits;

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
      this->numFractionalBits = bit2.numFractionalBits;

      for(s32 i=0; i<bit2.probeLocations.get_size(); i++) {
        this->probeLocations[i] = bit2.probeLocations[i];
        this->probeWeights[i] = bit2.probeWeights[i];
      }

      return *this;
    } // FiducialMarkerParserBit& FiducialMarkerParserBit::operator= (const FiducialMarkerParserBit& bit2)

    Result FiducialMarkerParserBit::ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, s16 &meanValue)
    {
      s32 accumulator = 0;

      const f64 h00 = homography[0][0];
      const f64 h10 = homography[1][0];
      const f64 h20 = homography[2][0];
      const f64 h01 = homography[0][1];
      const f64 h11 = homography[1][1];
      const f64 h21 = homography[2][1];
      const f64 h02 = homography[0][2];
      const f64 h12 = homography[1][2];
      const f64 h22 = homography[2][2];

      const f64 fixedPointDivider = 1.0 / pow(2,this->numFractionalBits);

      for(s32 probe=0; probe<probeLocations.get_size(); probe++) {
        const f64 x = static_cast<f64>(this->probeLocations[probe].x) * fixedPointDivider;
        const f64 y = static_cast<f64>(this->probeLocations[probe].y) * fixedPointDivider;
        const s16 weight = this->probeWeights[probe];

        // 1. Map each probe to its warped locations
        const f64 homogenousDivisor = 1.0 / (h20*x + h21*y + h22);

        const s16 warpedX = static_cast<s16>(Round( (h00 * x + h10 *y + h20) * homogenousDivisor ));
        const s16 warpedY = static_cast<s16>(Round( (h01 * x + h11 *y + h21) * homogenousDivisor ));

        // 2. Sample the image

        // This should only fail if there's a bug in the quad extraction
        assert(warpedY >= 0  && warpedX >= 0 && warpedY < image.get_size(0) && warpedX < image.get_size(1));

        const s16 imageValue = static_cast<s16>(image[warpedY][warpedX]);

        accumulator += weight * imageValue;
      }

      meanValue = (accumulator >> this->numFractionalBits);

      return RESULT_OK;
    } // Result FiducialMarkerParserBit::ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, s16 &meanValue)

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

    s32 FiducialMarkerParserBit::get_numFractionalBits() const
    {
      return this->numFractionalBits;
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

    Result FiducialMarkerParser::ExtractBlockMarker(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, BlockMarker &marker)
    {
      s16 meanValues[MAX_FIDUCIAL_MARKER_BITS];

      for(s32 bit=0; bit<bits.get_size(); bit++) {
        if(bits[bit].ExtractMeanValue(image, quad, homography, meanValues[bit]) != RESULT_OK)
          return RESULT_FAIL;
      }

      // TODO: finish parsing
      //function this = Marker2D(img, corners_, tform)
      //      % Note that corners are provided in the following order:
      //      %   1. Upper left
      //      %   2. Lower left
      //      %   3. Upper right
      //      %   4. Lower right
      //
      //      this.corners = corners_;
      //      this.ids = zeros(1, this.numIDs);
      //
      //      this.means = probeMeans(this, img, tform);
      //      [this, binaryString] = orientAndThreshold(this, this.means);
      //      if this.isValid
      //          this = decodeIDs(this, binaryString);
      //      end
      //
      //  end % CONSTRUCTOR Marker2D()

      return RESULT_OK;
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
          this->bits.PushBack(FiducialMarkerParserBit(probesX_type0[i], probesY_type0[i], probeWeights_type0[i], NUM_PROBES_PER_BIT_TYPE_0, bitTypes_type0[i], NUM_FRACTIONAL_BITS_TYPE_0));
        }
      } // if(INITIALIZE_WITH_DEFINITION_TYPE == 0)
      return RESULT_OK;
    }
  } // namespace Embedded
} // namespace Anki