#include "anki/embeddedVision/fiducialMarkers.h"

#include "anki/embeddedVision/fixedLengthList_vision.h"
#include "anki/embeddedVision/miscVisionKernels.h"
#include "anki/embeddedVision/draw_vision.h"

#include "fiducialMarkerDefinitionType0.h"

#include <assert.h>

#define INITIALIZE_WITH_DEFINITION_TYPE 0
#define NUM_BITS 25 // TODO: make general

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

    FiducialMarkerParserBit::FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probeWeights, const s32 numProbes, const FiducialMarkerParserBit::Type type, const s32 numFractionalBits)
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

    Result FiducialMarkerParserBit::ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, s16 &meanValue) const
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

      const f64 fixedPointDivider = 1.0 / pow(2.0, this->numFractionalBits);

      //#define SEND_WARPED_LOCATIONS
#ifdef SEND_WARPED_LOCATIONS
      Matlab matlab(false);
      //matlab.EvalStringEcho("warpedPoints = zeros(2, %d);", probeLocations.get_size());
      matlab.EvalStringEcho("if ~exist('warpedPoints', 'var') warpedPoints = zeros(2, 0); end;");
#endif

      for(s32 probe=0; probe<probeLocations.get_size(); probe++) {
        const f64 x = static_cast<f64>(this->probeLocations[probe].x) * fixedPointDivider;
        const f64 y = static_cast<f64>(this->probeLocations[probe].y) * fixedPointDivider;
        const s16 weight = this->probeWeights[probe];

        // 1. Map each probe to its warped locations
        const f64 homogenousDivisor = 1.0 / (h20*x + h21*y + h22);

        const f64 warpedXf = (h00 * x + h01 *y + h02) * homogenousDivisor;
        const f64 warpedYf = (h10 * x + h11 *y + h12) * homogenousDivisor;

        const s16 warpedX = static_cast<s16>(Round(warpedXf));
        const s16 warpedY = static_cast<s16>(Round(warpedYf));

#ifdef SEND_WARPED_LOCATIONS
        matlab.EvalStringEcho("warpedPoints(:,end+1) = [%f, %f];", warpedXf, warpedYf);
#endif

        // 2. Sample the image

        // This should only fail if there's a bug in the quad extraction
        assert(warpedY >= 0  && warpedX >= 0 && warpedY < image.get_size(0) && warpedX < image.get_size(1));

        const s16 imageValue = static_cast<s16>(image[warpedY][warpedX]);

        accumulator += weight * imageValue;
      }

      meanValue = (accumulator >> this->numFractionalBits);

      return RESULT_OK;
    } // Result FiducialMarkerParserBit::ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, s16 &meanValue)

    const FixedLengthList<Point<s16> >& FiducialMarkerParserBit::get_probeLocations() const
    {
      return this->probeLocations;
    }

    const FixedLengthList<s16>& FiducialMarkerParserBit::get_probeWeights() const
    {
      return this->probeWeights;
    }

    FiducialMarkerParserBit::Type FiducialMarkerParserBit::get_type() const
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

    // quad must have corners in the following order:
    //  1. Upper left
    //  2. Lower left
    //  3. Upper right
    //  4. Lower right
    Result FiducialMarkerParser::ExtractBlockMarker(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, const f32 minContrastRatio, BlockMarker &marker, MemoryStack scratch) const
    {
      FixedLengthList<s16> meanValues(MAX_FIDUCIAL_MARKER_BITS, scratch);

      marker.blockType = -1;
      marker.faceType = -1;
      marker.corners = quad;

      meanValues.set_size(bits.get_size());

      //#define SEND_PROBE_LOCATIONS

#ifdef SEND_PROBE_LOCATIONS
      {
        Matlab matlab(false);
        matlab.EvalStringEcho("clear");
        matlab.PutArray(image, "image");
      }
#endif

      for(s32 bit=0; bit<bits.get_size(); bit++) {
        if(bits[bit].ExtractMeanValue(image, quad, homography, meanValues[bit]) != RESULT_OK)
          return RESULT_FAIL;
      }

#ifdef SEND_PROBE_LOCATIONS
      {
        Matlab matlab(false);

        matlab.EvalStringEcho("probeLocations = zeros(2,0);");
        for(s32 i=0; i<bits.get_size(); i++) {
          FixedLengthList<Point<s16> > probeLocations = this->bits[i].get_probeLocations();
          matlab.Put(probeLocations.Pointer(0), probeLocations.get_size(), "probeLocationsTmp");
          matlab.EvalStringEcho("probeLocations(:, (end+1):(end+size(probeLocationsTmp,2))) = probeLocationsTmp;");
        }

        matlab.EvalStringEcho("probeLocations = double(probeLocations) / (2^%d)", this->bits[0].get_numFractionalBits());
      }
#endif // #ifdef SEND_PROBE_LOCATIONS

      FixedLengthList<u8> binarizedBits(MAX_FIDUCIAL_MARKER_BITS, scratch);

      // [this, binaryString] = orientAndThreshold(this, this.means);
      if(FiducialMarkerParser::DetermineOrientationAndBinarizeAndReorderCorners(meanValues, minContrastRatio, marker, binarizedBits, scratch) != RESULT_OK)
        return RESULT_FAIL;

      // meanValues.Print("meanValues");

      if(marker.orientation == BlockMarker::ORIENTATION_UNKNOWN)
        return RESULT_OK; // It couldn't be parsed, but this is not a code failure

      // this = decodeIDs(this, binaryString);
      if(DecodeId(binarizedBits, marker.blockType, marker.faceType, scratch) != RESULT_OK)
        return RESULT_FAIL;

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

      upBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_UP, 0);
      downBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_DOWN, 0);
      leftBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_LEFT, 0);
      rightBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_RIGHT, 0);

      // This should only fail if there was an issue with the FiducialMarkerParser creation
      assert(upBitIndex >= 0 && downBitIndex >= 0 && leftBitIndex >= 0 && rightBitIndex >= 0);

      return RESULT_OK;
    }

    Result FiducialMarkerParser::DetermineOrientationAndBinarizeAndReorderCorners(const FixedLengthList<s16> &meanValues, const f32 minContrastRatio, BlockMarker &marker, FixedLengthList<u8> &binarizedBits, MemoryStack scratch) const
    {
      AnkiConditionalErrorAndReturnValue(meanValues.IsValid(),
        RESULT_FAIL, "FiducialMarkerParser::DetermineOrientation", "meanValues is not valid");

      AnkiConditionalErrorAndReturnValue(binarizedBits.IsValid(),
        RESULT_FAIL, "FiducialMarkerParser::DetermineOrientation", "binarizedBits is not valid");

      binarizedBits.Clear();

      const s16 upBitValue = meanValues[upBitIndex];
      const s16 downBitValue = meanValues[downBitIndex];
      const s16 leftBitValue = meanValues[leftBitIndex];
      const s16 rightBitValue = meanValues[rightBitIndex];

      const s16 maxValue = MAX(upBitValue, MAX(downBitValue, MAX(leftBitValue, rightBitValue)));
      const s16 brightValue = maxValue;
      s16 darkValue;

      assert(meanValues.get_size() == NUM_BITS);

      FixedLengthList<u8> bitReadingOrder(meanValues.get_size(), scratch);

      // Note: this won't find ties, but that should be rare
      if(upBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_UP;
        darkValue = (downBitValue + leftBitValue + rightBitValue) / 3;

        const u8 readingOrder[NUM_BITS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
        bitReadingOrder.Set(readingOrder, NUM_BITS);
      } else if(downBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_DOWN;
        marker.corners = Quadrilateral<s16>(marker.corners[3], marker.corners[2], marker.corners[1], marker.corners[0]);
        darkValue = (upBitValue + leftBitValue + rightBitValue) / 3;

        const u8 readingOrder[NUM_BITS] = {24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
        bitReadingOrder.Set(readingOrder, NUM_BITS);
      } else if(leftBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_LEFT;
        marker.corners = Quadrilateral<s16>(marker.corners[1], marker.corners[3], marker.corners[0], marker.corners[2]);
        darkValue = (upBitValue + downBitValue + rightBitValue) / 3;

        const u8 readingOrder[NUM_BITS] = {4, 9, 14, 19, 24, 3, 8, 13, 18, 23, 2, 7, 12, 17, 22, 1, 6, 11, 16, 21, 0, 5, 10, 15, 20};
        bitReadingOrder.Set(readingOrder, NUM_BITS);
      } else {
        marker.orientation = BlockMarker::ORIENTATION_RIGHT;
        marker.corners = Quadrilateral<s16>(marker.corners[2], marker.corners[0], marker.corners[3], marker.corners[1]);
        darkValue = (upBitValue + downBitValue + leftBitValue) / 3;

        const u8 readingOrder[NUM_BITS] = {20, 15, 10, 5, 0, 21, 16, 11, 6, 1, 22, 17, 12, 7, 2, 23, 18, 13, 8, 3, 24, 19, 14, 9, 4};
        bitReadingOrder.Set(readingOrder, NUM_BITS);
      }

      if(static_cast<f32>(brightValue) < minContrastRatio * static_cast<f32>(darkValue)) {
        marker.orientation = BlockMarker::ORIENTATION_UNKNOWN;
        return RESULT_OK; // Low contrast is not really a failure, as it may be due to an invalid detection
      }

      const u8 threshold = static_cast<u8>( (brightValue + darkValue) / 2 );

      binarizedBits.set_size(bits.get_size());

      for(s32 i=0; i<NUM_BITS; i++) {
        const s32 index = bitReadingOrder[i];

        if(meanValues[index] < threshold)
          binarizedBits[i] = 1;
        else
          binarizedBits[i] = 0;
      }

      return RESULT_OK;
    }

    // Starting at startIndex, search through this->bits to find the first instance of the given type
    // Returns -1 if the type wasn't found
    s32 FiducialMarkerParser::FindFirstBitOfType(const FiducialMarkerParserBit::Type type, const s32 startIndex) const
    {
      AnkiConditionalErrorAndReturnValue(startIndex >= 0,
        -1, "FiducialMarkerParser::FindFirstBitOfType", "startIndex < 0");

      for(s32 i=startIndex;i<bits.get_size(); i++) {
        if(bits[i].get_type() == type)
          return i;
      } // for(s32 i=startIndex;i<bits.get_size(); i++)

      return -1;
    }

    Result FiducialMarkerParser::DecodeId(const FixedLengthList<u8> &binarizedBits, s16 &blockType, s16 &faceType, MemoryStack scratch) const
    {
      blockType = -1;
      faceType = -1;

      FixedLengthList<u8> checksumBits(8, scratch);
      FixedLengthList<u8> blockBits(8, scratch);
      FixedLengthList<u8> faceBits(4, scratch);

      // Convert the bit string in binarizedBits to numbers blockType and
      for(s32 bit=0; bit<binarizedBits.get_size(); bit++) {
        if(bits[bit].get_type() == FiducialMarkerParserBit::FIDUCIAL_BIT_BLOCK) {
          blockBits.PushBack(binarizedBits[bit]);
        } else if(bits[bit].get_type() == FiducialMarkerParserBit::FIDUCIAL_BIT_FACE) {
          faceBits.PushBack(binarizedBits[bit]);
        } else if(bits[bit].get_type() == FiducialMarkerParserBit::FIDUCIAL_BIT_CHECKSUM) {
          checksumBits.PushBack(binarizedBits[bit]);
        }
      }

      //#define DISPLAY_BITS
#ifdef DISPLAY_BITS
      blockBits.Print("blockBits");
      faceBits.Print("faceBits");
      checksumBits.Print("checksumBits");
#endif

      // Ids should start at 1
      blockType = 1 + BinaryStringToUnsignedNumber(blockBits, false);
      faceType = 1 + BinaryStringToUnsignedNumber(faceBits, false);

      if(!IsChecksumValid(checksumBits, blockBits, faceBits)) {
        blockType = -1;
        faceType = -1;
      }

      return RESULT_OK;
    }

    bool FiducialMarkerParser::IsChecksumValid(const FixedLengthList<u8> &checksumBits, const FixedLengthList<u8> &blockBits, const FixedLengthList<u8> &faceBits)
    {
      const s32 numBlockBits = blockBits.get_size();
      const s32 numFaceBits  = faceBits.get_size();

      s32 i_block1 = 1;

      //for i_check = 1:numCheckBits
      for(s32 i_check=1; i_check<=checksumBits.get_size(); i_check++) {
        //i_block2 = mod(i_block1, numBlockBits) + 1;
        //i_face = mod(i_check-1, numFaceBits) + 1;
        const s32 i_block2 = ((i_block1) % numBlockBits) + 1;
        const s32 i_face = ((i_check-1) % numFaceBits) + 1;

        const s32 expectedChecksumBit = faceBits[i_face-1] ^ (blockBits[i_block1-1] ^ blockBits[i_block2-1]);

        if(checksumBits[i_check-1] != expectedChecksumBit)
          return false;

        i_block1 = (i_block1 % numBlockBits) + 1;
      }

      return true;
    }
  } // namespace Embedded
} // namespace Anki