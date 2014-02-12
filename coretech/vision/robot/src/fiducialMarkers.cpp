/**
File: fidicialMarkers.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/fiducialMarkers.h"

#include "anki/vision/robot/miscVisionKernels.h"
#include "anki/vision/robot/draw_vision.h"

#include "fiducialMarkerDefinitionType0.h"

#include <assert.h>

#define INITIALIZE_WITH_DEFINITION_TYPE 0
#define NUM_BITS 25 // TODO: make general

namespace Anki
{
  namespace Embedded
  {
    static Result lastResult;

    BlockMarker::BlockMarker()
    {
    } // BlockMarker::BlockMarker()

    void BlockMarker::Print() const
    {
      printf("[%d,%d: (%d,%d) (%d,%d) (%d,%d) (%d,%d)] ", blockType, faceType, corners[0].x, corners[0].y, corners[1].x, corners[1].y, corners[2].x, corners[2].y, corners[3].x, corners[3].y);
    }

    FiducialMarkerParserBit::FiducialMarkerParserBit(MemoryStack &memory)
    {
      this->type = FIDUCIAL_BIT_UNINITIALIZED;

      this->probeLocations = FixedLengthList<Point<s16> >(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, memory);
      this->probeWeights = FixedLengthList<s16>(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, memory);
    }

    FiducialMarkerParserBit::FiducialMarkerParserBit(const FiducialMarkerParserBit& bit2)
    {
      this->probeLocations = bit2.probeLocations;
      this->probeWeights = bit2.probeWeights;

      AnkiAssert(bit2.probeLocations.get_size() == bit2.probeWeights.get_size());

      this->probeLocations.set_size(bit2.probeLocations.get_size());
      this->probeWeights.set_size(bit2.probeWeights.get_size());
      this->type = bit2.type;
      this->numFractionalBits = bit2.numFractionalBits;

      const s32 numBit2ProbeLocations = bit2.probeLocations.get_size();
      for(s32 i=0; i<numBit2ProbeLocations; i++) {
        this->probeLocations[i] = bit2.probeLocations[i];
        this->probeWeights[i] = bit2.probeWeights[i];
      }
    } // FiducialMarkerParserBit::FiducialMarkerParserBit(const FiducialMarkerParserBit& bit2)

    FiducialMarkerParserBit::FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probeWeights, const s32 numProbes, const FiducialMarkerParserBit::Type type, const s32 numFractionalBits, MemoryStack &memory)
    {
      AnkiAssert(numProbes <= MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS);

      this->type = FIDUCIAL_BIT_UNINITIALIZED;

      this->probeLocations = FixedLengthList<Point<s16> >(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, memory);
      this->probeWeights = FixedLengthList<s16>(MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS, memory);

      this->probeLocations.set_size(numProbes);
      this->probeWeights.set_size(numProbes);
      this->type = type;
      this->numFractionalBits = numFractionalBits;

      const s32 numProbeLocations = probeLocations.get_size();
      for(s32 i=0; i<numProbeLocations; i++) {
        this->probeLocations[i].x = probesX[i];
        this->probeLocations[i].y = probesY[i];
        this->probeWeights[i] = probeWeights[i];
      }
    } // FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probesWeights, const s32 numProbes)

    FiducialMarkerParserBit& FiducialMarkerParserBit::operator= (const FiducialMarkerParserBit& bit2)
    {
      AnkiAssert(bit2.probeLocations.get_size() == bit2.probeWeights.get_size());

      this->probeLocations = bit2.probeLocations;
      this->probeWeights = bit2.probeWeights;

      this->probeLocations.set_size(bit2.probeLocations.get_size());
      this->probeWeights.set_size(bit2.probeWeights.get_size());
      this->type = bit2.type;
      this->numFractionalBits = bit2.numFractionalBits;

      const s32 numBit2ProbeLocations = bit2.probeLocations.get_size();
      for(s32 i=0; i<numBit2ProbeLocations; i++) {
        this->probeLocations[i] = bit2.probeLocations[i];
        this->probeWeights[i] = bit2.probeWeights[i];
      }

      return *this;
    } // FiducialMarkerParserBit& FiducialMarkerParserBit::operator= (const FiducialMarkerParserBit& bit2)

    Result FiducialMarkerParserBit::ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, s16 &meanValue) const
    {
      s32 accumulator = 0;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

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

      const s32 numProbeLocations = probeLocations.get_size();
      for(s32 probe=0; probe<numProbeLocations; probe++) {
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
        AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < imageHeight && warpedX < imageWidth);

        // This is the direct way to access a pixel. It doesn't work when there's an endian conflict
        //const s16 imageValue = static_cast<s16>(*image.Pointer(warpedY, warpedX));

        // This is the indirect way. It reads a whole 32-bit word, then extracts the correct byte
        const u32 * restrict pImageY = reinterpret_cast<const u32*>(image.Pointer(warpedY,0));
        const s32 xWord = warpedX >> 2;

        const s32 xByte = warpedX - (xWord << 2);

        const u32 curPixelWord = pImageY[xWord];
        const u8 curPixel = (curPixelWord & (0xFF << (8*xByte))) >> (8*xByte);
        const s16 imageValue = static_cast<s16>(curPixel);

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

    /*void FiducialMarkerParserBit::PrepareBuffers()
    {
    }*/

    // Initialize with the default grid type, converted from Matlab
    FiducialMarkerParser::FiducialMarkerParser(MemoryStack &memory)
    {
      this->bits = FixedLengthList<FiducialMarkerParserBit>(MAX_FIDUCIAL_MARKER_BITS, memory);
      InitializeAsDefaultParser(memory);
    }

    FiducialMarkerParser::FiducialMarkerParser(const FiducialMarkerParser& marker2)
    {
      this->bits = marker2.bits;
    }

    // quad must have corners in the following order:
    //  1. Upper left
    //  2. Lower left
    //  3. Upper right
    //  4. Lower right
    Result FiducialMarkerParser::ExtractBlockMarker(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f64> &homography, const f32 minContrastRatio, BlockMarker &marker, MemoryStack scratch) const
    {
      FixedLengthList<s16> meanValues(MAX_FIDUCIAL_MARKER_BITS, scratch);

      const s32 numBits = bits.get_size();

      marker.blockType = -1;
      marker.faceType = -1;
      marker.corners = quad;

      meanValues.set_size(numBits);

      //#define SEND_PROBE_LOCATIONS

#ifdef SEND_PROBE_LOCATIONS
      {
        Matlab matlab(false);
        matlab.EvalStringEcho("clear");
        matlab.PutArray(image, "image");
      }
#endif

      for(s32 bit=0; bit<numBits; bit++) {
        if((lastResult = bits[bit].ExtractMeanValue(image, quad, homography, meanValues[bit])) != RESULT_OK)
          return lastResult;
      }

#ifdef SEND_PROBE_LOCATIONS
      {
        Matlab matlab(false);

        matlab.EvalStringEcho("probeLocations = zeros(2,0);");
        for(s32 i=0; i<numBits; i++) {
          FixedLengthList<Point<s16> > probeLocations = this->bits[i].get_probeLocations();
          matlab.Put(probeLocations.Pointer(0), probeLocations.get_size(), "probeLocationsTmp");
          matlab.EvalStringEcho("probeLocations(:, (end+1):(end+size(probeLocationsTmp,2))) = probeLocationsTmp;");
        }

        matlab.EvalStringEcho("probeLocations = double(probeLocations) / (2^%d)", this->bits[0].get_numFractionalBits());
      }
#endif // #ifdef SEND_PROBE_LOCATIONS

      FixedLengthList<u8> binarizedBits(MAX_FIDUCIAL_MARKER_BITS, scratch);

      // [this, binaryString] = orientAndThreshold(this, this.means);
      if((lastResult = FiducialMarkerParser::DetermineOrientationAndBinarizeAndReorderCorners(meanValues, minContrastRatio, marker, binarizedBits, scratch)) != RESULT_OK)
        return lastResult;

      // meanValues.Print("meanValues");

      if(marker.orientation == BlockMarker::ORIENTATION_UNKNOWN)
        return RESULT_OK; // It couldn't be parsed, but this is not a code failure

      // this = decodeIDs(this, binaryString);
      if((lastResult = DecodeId(binarizedBits, marker.blockType, marker.faceType, scratch)) != RESULT_OK)
        return lastResult;

      return RESULT_OK;
    }

    FiducialMarkerParser& FiducialMarkerParser::operator= (const FiducialMarkerParser& marker2)
    {
      this->bits = marker2.bits;

      return *this;
    }

    Result FiducialMarkerParser::InitializeAsDefaultParser(MemoryStack &memory)
    {
      if(INITIALIZE_WITH_DEFINITION_TYPE == 0) {
        AnkiAssert(NUM_BITS_TYPE_0 <= MAX_FIDUCIAL_MARKER_BITS);
        AnkiAssert(NUM_PROBES_PER_BIT_TYPE_0 <= MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS);

        this->bits.Clear();

        for(s32 i=0; i<NUM_BITS_TYPE_0; i++) {
          this->bits.PushBack(FiducialMarkerParserBit(probesX_type0[i], probesY_type0[i], probeWeights_type0[i], NUM_PROBES_PER_BIT_TYPE_0, bitTypes_type0[i], NUM_FRACTIONAL_BITS_TYPE_0, memory));
        }
      } // if(INITIALIZE_WITH_DEFINITION_TYPE == 0)

      upBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_UP, 0);
      downBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_DOWN, 0);
      leftBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_LEFT, 0);
      rightBitIndex = FindFirstBitOfType(FiducialMarkerParserBit::FIDUCIAL_BIT_ORIENTATION_RIGHT, 0);

      // This should only fail if there was an issue with the FiducialMarkerParser creation
      AnkiAssert(upBitIndex >= 0 && downBitIndex >= 0 && leftBitIndex >= 0 && rightBitIndex >= 0);

      return RESULT_OK;
    }

    Result FiducialMarkerParser::DetermineOrientationAndBinarizeAndReorderCorners(const FixedLengthList<s16> &meanValues, const f32 minContrastRatio, BlockMarker &marker, FixedLengthList<u8> &binarizedBits, MemoryStack scratch) const
    {
      AnkiConditionalErrorAndReturnValue(meanValues.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "FiducialMarkerParser::DetermineOrientation", "meanValues is not valid");

      AnkiConditionalErrorAndReturnValue(binarizedBits.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "FiducialMarkerParser::DetermineOrientation", "binarizedBits is not valid");

      binarizedBits.Clear();

      const s16 upBitValue = meanValues[upBitIndex];
      const s16 downBitValue = meanValues[downBitIndex];
      const s16 leftBitValue = meanValues[leftBitIndex];
      const s16 rightBitValue = meanValues[rightBitIndex];

      const s16 maxValue = MAX(upBitValue, MAX(downBitValue, MAX(leftBitValue, rightBitValue)));
      const s16 brightValue = maxValue;
      s16 darkValue;

      AnkiAssert(meanValues.get_size() == NUM_BITS);

      FixedLengthList<u8> bitReadingOrder(meanValues.get_size(), scratch);
      bitReadingOrder.set_size(meanValues.get_size());

      // Note: this won't find ties, but that should be rare
      if(upBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_UP;
        darkValue = (downBitValue + leftBitValue + rightBitValue) / 3;

        bitReadingOrder[0] = 0; bitReadingOrder[1] = 1; bitReadingOrder[2] = 2; bitReadingOrder[3] = 3; bitReadingOrder[4] = 4; bitReadingOrder[5] = 5; bitReadingOrder[6] = 6; bitReadingOrder[7] = 7; bitReadingOrder[8] = 8; bitReadingOrder[9] = 9; bitReadingOrder[10] = 10; bitReadingOrder[11] = 11; bitReadingOrder[12] = 12; bitReadingOrder[13] = 13; bitReadingOrder[14] = 14; bitReadingOrder[15] = 15; bitReadingOrder[16] = 16; bitReadingOrder[17] = 17; bitReadingOrder[18] = 18; bitReadingOrder[19] = 19; bitReadingOrder[20] = 20; bitReadingOrder[21] = 21; bitReadingOrder[22] = 22; bitReadingOrder[23] = 23; bitReadingOrder[24] = 24;
      } else if(downBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_DOWN;
        marker.corners = Quadrilateral<s16>(marker.corners[3], marker.corners[2], marker.corners[1], marker.corners[0]);
        darkValue = (upBitValue + leftBitValue + rightBitValue) / 3;

        bitReadingOrder[0] = 24; bitReadingOrder[1] = 23; bitReadingOrder[2] = 22; bitReadingOrder[3] = 21; bitReadingOrder[4] = 20; bitReadingOrder[5] = 19; bitReadingOrder[6] = 18; bitReadingOrder[7] = 17; bitReadingOrder[8] = 16; bitReadingOrder[9] = 15; bitReadingOrder[10] = 14; bitReadingOrder[11] = 13; bitReadingOrder[12] = 12; bitReadingOrder[13] = 11; bitReadingOrder[14] = 10; bitReadingOrder[15] = 9; bitReadingOrder[16] = 8; bitReadingOrder[17] = 7; bitReadingOrder[18] = 6; bitReadingOrder[19] = 5; bitReadingOrder[20] = 4; bitReadingOrder[21] = 3; bitReadingOrder[22] = 2; bitReadingOrder[23] = 1; bitReadingOrder[24] = 0;

        //printf("readingOrder:\n");
        //for(s32 i=0; i<NUM_BITS; i++) {
        //  printf("(%d) ", bitReadingOrder[i]);
        //}
        //printf("\n");
      } else if(leftBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_LEFT;
        marker.corners = Quadrilateral<s16>(marker.corners[1], marker.corners[3], marker.corners[0], marker.corners[2]);
        darkValue = (upBitValue + downBitValue + rightBitValue) / 3;

        bitReadingOrder[0] = 4; bitReadingOrder[1] = 9; bitReadingOrder[2] = 14; bitReadingOrder[3] = 19; bitReadingOrder[4] = 24; bitReadingOrder[5] = 3; bitReadingOrder[6] = 8; bitReadingOrder[7] = 13; bitReadingOrder[8] = 18; bitReadingOrder[9] = 23; bitReadingOrder[10] = 2; bitReadingOrder[11] = 7; bitReadingOrder[12] = 12; bitReadingOrder[13] = 17; bitReadingOrder[14] = 22; bitReadingOrder[15] = 1; bitReadingOrder[16] = 6; bitReadingOrder[17] = 11; bitReadingOrder[18] = 16; bitReadingOrder[19] = 21; bitReadingOrder[20] = 0; bitReadingOrder[21] = 5; bitReadingOrder[22] = 10; bitReadingOrder[23] = 15; bitReadingOrder[24] = 20;

        //printf("readingOrder:\n");
        //for(s32 i=0; i<NUM_BITS; i++) {
        //  printf("(%d) ", bitReadingOrder[i]);
        //}
        //printf("\n");
      } else {
        marker.orientation = BlockMarker::ORIENTATION_RIGHT;
        marker.corners = Quadrilateral<s16>(marker.corners[2], marker.corners[0], marker.corners[3], marker.corners[1]);
        darkValue = (upBitValue + downBitValue + leftBitValue) / 3;

        bitReadingOrder[0] = 20; bitReadingOrder[1] = 15; bitReadingOrder[2] = 10; bitReadingOrder[3] = 5; bitReadingOrder[4] = 0; bitReadingOrder[5] = 21; bitReadingOrder[6] = 16; bitReadingOrder[7] = 11; bitReadingOrder[8] = 6; bitReadingOrder[9] = 1; bitReadingOrder[10] = 22; bitReadingOrder[11] = 17; bitReadingOrder[12] = 12; bitReadingOrder[13] = 7; bitReadingOrder[14] = 2; bitReadingOrder[15] = 23; bitReadingOrder[16] = 18; bitReadingOrder[17] = 13; bitReadingOrder[18] = 8; bitReadingOrder[19] = 3; bitReadingOrder[20] = 24; bitReadingOrder[21] = 19; bitReadingOrder[22] = 14; bitReadingOrder[23] = 9; bitReadingOrder[24] = 4;

        //printf("readingOrder:\n");
        //for(s32 i=0; i<NUM_BITS; i++) {
        //  printf("(%d) ", bitReadingOrder[i]);
        //}
        //printf("\n");
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

      const s32 numBits = bits.get_size();
      for(s32 i=startIndex;i<numBits; i++) {
        if(bits[i].get_type() == type)
          return i;
      } // for(s32 i=startIndex;i<numBits; i++)

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
      const s32 numBinarizedBits = binarizedBits.get_size();
      for(s32 bit=0; bit<numBinarizedBits; bit++) {
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
      const s32 numChecksumBits = checksumBits.get_size();
      for(s32 i_check=1; i_check<=numChecksumBits; i_check++) {
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
