/**
File: fidicialMarkers.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/fiducialMarkers.h"

#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/robot/serialize.h"

#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/draw_vision.h"

#include "coretech/vision/robot/transformations.h"

#include "fiducialMarkerDefinitionType0.h"

#include <assert.h>

#if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR || RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
#  include "engine/vision/nearestNeighborLibraryData.h"
#endif 

#define INITIALIZE_WITH_DEFINITION_TYPE 0
//#define NUM_BITS 25 // TODO: make general
#define NUM_BITS MAX_FIDUCIAL_MARKER_BITS // TODO: Why do we need a separate NUM_BITS?

#define OUTPUT_FAILED_MARKER_STEPS

#define DO_RECOGNITION_TIMING 0

#if defined(THIS_IS_PETES_BOARD)
#undef OUTPUT_FAILED_MARKER_STEPS
#endif

//#define SHOW_EXHAUSTIVE_STEPS

#if !ANKICORETECH_EMBEDDED_USE_OPENCV
#undef SHOW_EXHAUSTIVE_STEPS
#endif

#define DEBUG_BRIGHT_DARK_PAIRS 0


#include <chrono>

namespace Anki
{
  namespace Embedded
  {
#   if RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
    FiducialMarkerDecisionTree VisionMarker::multiClassTrees[VisionMarkerDecisionTree::NUM_TREES];

    bool VisionMarker::areTreesInitialized = false;
#   endif
    
    BlockMarker::BlockMarker()
    {
    } // BlockMarker::BlockMarker()

    void BlockMarker::Print() const
    {
      CoreTechPrint("[%d,%d: (%d,%d) (%d,%d) (%d,%d) (%d,%d)] ", blockType, faceType, corners[0].x, corners[0].y, corners[1].x, corners[1].y, corners[2].x, corners[2].y, corners[3].x, corners[3].y);
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

    Result FiducialMarkerParserBit::ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f32> &homography, s16 &meanValue) const
    {
      s32 accumulator = 0;

      const f32 h00 = homography[0][0];
      const f32 h10 = homography[1][0];
      const f32 h20 = homography[2][0];
      const f32 h01 = homography[0][1];
      const f32 h11 = homography[1][1];
      const f32 h21 = homography[2][1];
      const f32 h02 = homography[0][2];
      const f32 h12 = homography[1][2];
      const f32 h22 = homography[2][2];

      const f32 fixedPointDivider = 1.0f / static_cast<f32>(1 << this->numFractionalBits);

      //#define SEND_WARPED_LOCATIONS
#ifdef SEND_WARPED_LOCATIONS
      Matlab matlab(false);
      matlab.EvalStringEcho("if ~exist('warpedPoints', 'var') warpedPoints = zeros(2, 0); end;");
      //matlab.EvalStringEcho("warpedPoints = zeros(2, 0);");
#endif

      const Point<s16> * restrict pProbeLocations = this->probeLocations.Pointer(0);
      const s16 * restrict pProbeWeights = this->probeWeights.Pointer(0);

      const s32 numProbeLocations = probeLocations.get_size();
      for(s32 probe=0; probe<numProbeLocations; probe++) {
        const f32 x = static_cast<f32>(pProbeLocations[probe].x) * fixedPointDivider;
        const f32 y = static_cast<f32>(pProbeLocations[probe].y) * fixedPointDivider;
        const s16 weight = pProbeWeights[probe];

        // 1. Map each probe to its warped locations
        const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);

        const f32 warpedXf = (h00 * x + h01 *y + h02) * homogenousDivisor;
        const f32 warpedYf = (h10 * x + h11 *y + h12) * homogenousDivisor;

        const s32 warpedX = Round<s32>(warpedXf);
        const s32 warpedY = Round<s32>(warpedYf);

#ifdef SEND_WARPED_LOCATIONS
        matlab.EvalStringEcho("warpedPoints(:,end+1) = [%f, %f];", warpedXf, warpedYf);
#endif

        // 2. Sample the image

        // This should only fail if there's a bug in the quad extraction
        AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < image.get_size(0) && warpedX < image.get_size(1)); // Verify y against image height and x against width

        const s16 imageValue = static_cast<s16>(*image.Pointer(warpedY, warpedX));

        accumulator += weight * imageValue;
      }

      meanValue = (accumulator >> this->numFractionalBits);

      return RESULT_OK;
    } // Result FiducialMarkerParserBit::ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f32> &homography, s16 &meanValue)

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
    Result FiducialMarkerParser::ExtractBlockMarker(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f32> &homography, const f32 minContrastRatio, BlockMarker &marker, MemoryStack scratch) const
    {
      BeginBenchmark("fmpebm_init");

      Result lastResult;

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

      EndBenchmark("fmpebm_init");

      BeginBenchmark("fmpebm_extractMean");
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

      EndBenchmark("fmpebm_extractMean");

      BeginBenchmark("fmpebm_orient");
      FixedLengthList<u8> binarizedBits(MAX_FIDUCIAL_MARKER_BITS, scratch);

      // [this, binaryString] = orientAndThreshold(this, this.means);
      if((lastResult = FiducialMarkerParser::DetermineOrientationAndBinarizeAndReorderCorners(meanValues, minContrastRatio, marker, binarizedBits, scratch)) != RESULT_OK)
        return lastResult;

      // meanValues.Print("meanValues");

      if(marker.orientation == BlockMarker::ORIENTATION_UNKNOWN)
        return RESULT_OK; // It couldn't be parsed, but this is not a code failure

      EndBenchmark("fmpebm_orient");

      BeginBenchmark("fmpebm_decode");

      // this = decodeIDs(this, binaryString);
      if((lastResult = DecodeId(binarizedBits, marker.blockType, marker.faceType, scratch)) != RESULT_OK)
        return lastResult;

      EndBenchmark("fmpebm_decode");

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

      // NOTE: this won't find ties, but that should be rare
      if(upBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_UP;
        darkValue = (downBitValue + leftBitValue + rightBitValue) / 3;

        bitReadingOrder[0] = 0; bitReadingOrder[1] = 1; bitReadingOrder[2] = 2; bitReadingOrder[3] = 3; bitReadingOrder[4] = 4; bitReadingOrder[5] = 5; bitReadingOrder[6] = 6; bitReadingOrder[7] = 7; bitReadingOrder[8] = 8; bitReadingOrder[9] = 9; bitReadingOrder[10] = 10; bitReadingOrder[11] = 11; bitReadingOrder[12] = 12; bitReadingOrder[13] = 13; bitReadingOrder[14] = 14; bitReadingOrder[15] = 15; bitReadingOrder[16] = 16; bitReadingOrder[17] = 17; bitReadingOrder[18] = 18; bitReadingOrder[19] = 19; bitReadingOrder[20] = 20; bitReadingOrder[21] = 21; bitReadingOrder[22] = 22; bitReadingOrder[23] = 23; bitReadingOrder[24] = 24;
      } else if(downBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_DOWN;
        marker.corners = Quadrilateral<s16>(marker.corners[3], marker.corners[2], marker.corners[1], marker.corners[0]);
        darkValue = (upBitValue + leftBitValue + rightBitValue) / 3;

        bitReadingOrder[0] = 24; bitReadingOrder[1] = 23; bitReadingOrder[2] = 22; bitReadingOrder[3] = 21; bitReadingOrder[4] = 20; bitReadingOrder[5] = 19; bitReadingOrder[6] = 18; bitReadingOrder[7] = 17; bitReadingOrder[8] = 16; bitReadingOrder[9] = 15; bitReadingOrder[10] = 14; bitReadingOrder[11] = 13; bitReadingOrder[12] = 12; bitReadingOrder[13] = 11; bitReadingOrder[14] = 10; bitReadingOrder[15] = 9; bitReadingOrder[16] = 8; bitReadingOrder[17] = 7; bitReadingOrder[18] = 6; bitReadingOrder[19] = 5; bitReadingOrder[20] = 4; bitReadingOrder[21] = 3; bitReadingOrder[22] = 2; bitReadingOrder[23] = 1; bitReadingOrder[24] = 0;

        //CoreTechPrint("readingOrder:\n");
        //for(s32 i=0; i<NUM_BITS; i++) {
        //  CoreTechPrint("(%d) ", bitReadingOrder[i]);
        //}
        //CoreTechPrint("\n");
      } else if(leftBitValue == maxValue) {
        marker.orientation = BlockMarker::ORIENTATION_LEFT;
        marker.corners = Quadrilateral<s16>(marker.corners[1], marker.corners[3], marker.corners[0], marker.corners[2]);
        darkValue = (upBitValue + downBitValue + rightBitValue) / 3;

        bitReadingOrder[0] = 4; bitReadingOrder[1] = 9; bitReadingOrder[2] = 14; bitReadingOrder[3] = 19; bitReadingOrder[4] = 24; bitReadingOrder[5] = 3; bitReadingOrder[6] = 8; bitReadingOrder[7] = 13; bitReadingOrder[8] = 18; bitReadingOrder[9] = 23; bitReadingOrder[10] = 2; bitReadingOrder[11] = 7; bitReadingOrder[12] = 12; bitReadingOrder[13] = 17; bitReadingOrder[14] = 22; bitReadingOrder[15] = 1; bitReadingOrder[16] = 6; bitReadingOrder[17] = 11; bitReadingOrder[18] = 16; bitReadingOrder[19] = 21; bitReadingOrder[20] = 0; bitReadingOrder[21] = 5; bitReadingOrder[22] = 10; bitReadingOrder[23] = 15; bitReadingOrder[24] = 20;

        //CoreTechPrint("readingOrder:\n");
        //for(s32 i=0; i<NUM_BITS; i++) {
        //  CoreTechPrint("(%d) ", bitReadingOrder[i]);
        //}
        //CoreTechPrint("\n");
      } else {
        marker.orientation = BlockMarker::ORIENTATION_RIGHT;
        marker.corners = Quadrilateral<s16>(marker.corners[2], marker.corners[0], marker.corners[3], marker.corners[1]);
        darkValue = (upBitValue + downBitValue + leftBitValue) / 3;

        bitReadingOrder[0] = 20; bitReadingOrder[1] = 15; bitReadingOrder[2] = 10; bitReadingOrder[3] = 5; bitReadingOrder[4] = 0; bitReadingOrder[5] = 21; bitReadingOrder[6] = 16; bitReadingOrder[7] = 11; bitReadingOrder[8] = 6; bitReadingOrder[9] = 1; bitReadingOrder[10] = 22; bitReadingOrder[11] = 17; bitReadingOrder[12] = 12; bitReadingOrder[13] = 7; bitReadingOrder[14] = 2; bitReadingOrder[15] = 23; bitReadingOrder[16] = 18; bitReadingOrder[17] = 13; bitReadingOrder[18] = 8; bitReadingOrder[19] = 3; bitReadingOrder[20] = 24; bitReadingOrder[21] = 19; bitReadingOrder[22] = 14; bitReadingOrder[23] = 9; bitReadingOrder[24] = 4;

        //CoreTechPrint("readingOrder:\n");
        //for(s32 i=0; i<NUM_BITS; i++) {
        //  CoreTechPrint("(%d) ", bitReadingOrder[i]);
        //}
        //CoreTechPrint("\n");
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

    
    std::string VisionMarker::_dataPath("");
    
    void VisionMarker::SetDataPath(const std::string& dataPath)
    {
      _dataPath = dataPath;
    }
    
    VisionMarker::VisionMarker()
    : corners(Point<f32>(-1.0f,-1.0f), Point<f32>(-1.0f,-1.0f), Point<f32>(-1.0f,-1.0f), Point<f32>(-1.0f,-1.0f))
    , markerType(Anki::Vision::MARKER_UNKNOWN)
    , observedOrientation(0)
    , validity(UNKNOWN)
    {
      Initialize();
    }

    VisionMarker::VisionMarker(const Quadrilateral<s16> &corners, const ValidityCode validity)
    : markerType(Anki::Vision::MARKER_UNKNOWN)
    , observedOrientation(0)
    , validity(validity)
    {
      this->corners.SetCast<s16>(corners);
      
      Initialize();
    } // VisionMarker::VisionMarker()

    VisionMarker::VisionMarker(const Quadrilateral<f32> &corners, const ValidityCode validity)
    : corners(corners)
    , markerType(Anki::Vision::MARKER_UNKNOWN)
    , observedOrientation(0)
    , validity(UNKNOWN)
    {
      Initialize();
    }

    void VisionMarker::Print() const
    {
      const char * typeString = "??";
      if(static_cast<s32>(markerType) >=0 && static_cast<s32>(markerType) <= Vision::NUM_MARKER_TYPES) {
        typeString = Vision::MarkerTypeStrings[markerType];
      }

      CoreTechPrint("[Type %d-%s]: (%0.2f,%0.2f) (%0.2f,%0.2f) (%0.2f,%0.2f) (%0.2f,%0.2f)] ",
        markerType, typeString,
        corners[0].x, corners[0].y,
        corners[1].x, corners[1].y,
        corners[2].x, corners[2].y,
        corners[3].x, corners[3].y);
    } // VisionMarker::Print()

    Result VisionMarker::Serialize(const char *objectName, SerializedBuffer &buffer) const
    {
      s32 totalDataLength = this->get_serializationSize();

      void *segment = buffer.Allocate("VisionMarker", objectName, totalDataLength);

      if(segment == NULL) {
        return RESULT_FAIL;
      }

      return SerializeRaw(objectName, &segment, totalDataLength);
    }

    Result VisionMarker::SerializeRaw(const char *objectName, void ** buffer, s32 &bufferLength) const
    {
      if(SerializedBuffer::SerializeDescriptionStrings("VisionMarker", objectName, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      SerializedBuffer::SerializeRawBasicType<Quadrilateral<f32> >("corners", this->corners, buffer, bufferLength);
      SerializedBuffer::SerializeRawBasicType<s32>("markerType", this->markerType, buffer, bufferLength);
      SerializedBuffer::SerializeRawBasicType<ValidityCode>("validity", this->validity, buffer, bufferLength);
      SerializedBuffer::SerializeRawBasicType<f32>("observedOrientation", this->observedOrientation, buffer, bufferLength);

      return RESULT_OK;
    }

    Result VisionMarker::Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack scratch)
    {
      // TODO: check if the name is correct
      if(SerializedBuffer::DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      this->corners = SerializedBuffer::DeserializeRawBasicType<Quadrilateral<f32> >(NULL, buffer, bufferLength);
      this->markerType = static_cast<Vision::MarkerType>(SerializedBuffer::DeserializeRawBasicType<s32>(NULL, buffer, bufferLength));
      this->validity = SerializedBuffer::DeserializeRawBasicType<ValidityCode>(NULL, buffer, bufferLength);
      this->observedOrientation = SerializedBuffer::DeserializeRawBasicType<f32>(NULL, buffer, bufferLength);

      return RESULT_OK;
    }

#   if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR
    NearestNeighborLibrary& VisionMarker::GetNearestNeighborLibrary()
    {
      /* Use data loaded from binary files, using data platform (needs work!)
      static NearestNeighborLibrary nearestNeighborLibrary(_dataPlatform,
                                                           NUM_MARKERS_IN_LIBRARY,
                                                           NUM_PROBES,
                                                           ProbeCenters_X, ProbeCenters_Y,
                                                           ProbePoints_X,  ProbePoints_Y,
                                                           NUM_PROBE_POINTS,
                                                           NUM_FRACTIONAL_BITS);
       */
      
      static NearestNeighborLibrary nearestNeighborLibrary(NearestNeighborData,
                                                           NearestNeighborWeights,
                                                           NearestNeighborLabels,
                                                           NUM_MARKERS_IN_LIBRARY,
                                                           NUM_PROBES,
                                                           ProbeCenters_X, ProbeCenters_Y,
                                                           ProbePoints_X,  ProbePoints_Y,
                                                           NUM_PROBE_POINTS,
                                                           NUM_FRACTIONAL_BITS);
      
      /*
      // HoG version:
      static NearestNeighborLibrary nearestNeighborLibrary(NearestNeighborData,
                                                           NearestNeighborLabels,
                                                           NUM_MARKERS_IN_LIBRARY,
                                                           NUM_PROBES,
                                                           ProbeCenters_X, ProbeCenters_Y,
                                                           ProbePoints_X,  ProbePoints_Y,
                                                           NUM_PROBE_POINTS,
                                                           NN_NUM_FRACTIONAL_BITS,
                                                           4, 8);
      */
      
      return nearestNeighborLibrary;
    }
    
#   elif RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
    
    Vision::ConvolutionalNeuralNet& VisionMarker::GetCNN()
    {
      static Vision::ConvolutionalNeuralNet cnn;
      if(!cnn.IsLoaded()) {
        AnkiConditionalErrorAndReturnValue(_dataPath.empty() == false, cnn,
                                           "VisionMarker.GetCNN.NoDataPath",
                                           "DataPath must be set before calling GetCNN.");
        
        Result loadResult = cnn.Load(_dataPath + "/visionMarkerCNN");

        AnkiConditionalError(loadResult == RESULT_OK,
                             "VisionMarker.GetCNN.LoadFailed",
                             "Failed to load CNN from '%s'", cnnDir.c_str());
      }
      return cnn;
    }
    
#   endif // #if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR

    
    void VisionMarker::Initialize()
    {     
#     if RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
      
      if(VisionMarker::areTreesInitialized == false) {
        using namespace VisionMarkerDecisionTree;

        // Initialize trees on first use
        for(u32 iTree = 0; iTree < NUM_TREES; ++iTree) {
          
          VisionMarker::multiClassTrees[iTree] = FiducialMarkerDecisionTree(reinterpret_cast<const u8*>(MultiClassNodes[iTree]),
                                                                            NUM_NODES_MULTICLASS[iTree],
                                                                            TREE_NUM_FRACTIONAL_BITS,
                                                                            MAX_DEPTH_MULTICLASS[iTree],
                                                                            ProbePoints_X, ProbePoints_Y,
                                                                            NUM_PROBE_POINTS, NULL, 0);
        }
        
        VisionMarker::areTreesInitialized = true;
      } // IF trees initialized

#     elif RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR

      // Nothing to do here?
      
#     elif RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
    
      // Nothing to do here?
      
#     endif // #if RECOGNITION_METHOD == ?
      
    }
    
    
    Result VisionMarker::GetProbeValues(const Array<u8> &image,
                                        const Array<f32> &homography,
                                        const bool doIlluminationNormalization,
                                        cv::Mat_<u8> &probeValues)
    {
      AnkiConditionalErrorAndReturnValue(AreValid(image, homography),
                                         RESULT_FAIL_INVALID_OBJECT, "VisionMarker::GetProbeValues", "Invalid objects");
      
      // This function is optimized for 9 or fewer probes, but this is not a big issue
      AnkiAssert(VisionMarker::NUM_PROBE_POINTS <= 9);
      
      const s32 numProbeOffsets = VisionMarker::NUM_PROBE_POINTS;
      const s32 numProbes = VisionMarker::NUM_PROBES;
      
      const f32 h00 = homography[0][0];
      const f32 h10 = homography[1][0];
      const f32 h20 = homography[2][0];
      const f32 h01 = homography[0][1];
      const f32 h11 = homography[1][1];
      const f32 h21 = homography[2][1];
      const f32 h02 = homography[0][2];
      const f32 h12 = homography[1][2];
      const f32 h22 = homography[2][2];
      
      const f32 fixedPointDivider = 1.0f / static_cast<f32>(1 << VisionMarker::NUM_FRACTIONAL_BITS);
      
      AnkiConditionalErrorAndReturnValue(probeValues.rows*probeValues.cols == VisionMarker::NUM_PROBES,
                                         RESULT_FAIL_INVALID_SIZE,
                                         "VisionMarker.GetProbeValues",
                                         "Output probeValues matrix should have %d elements, not %d",
                                         VisionMarker::NUM_PROBES, probeValues.rows*probeValues.cols);
      
      u8* restrict pProbeData = probeValues[0];
      
      f32 probeXOffsetsF32[9];
      f32 probeYOffsetsF32[9];
      
      for(s32 iOffset=0; iOffset<numProbeOffsets; iOffset++) {
        probeXOffsetsF32[iOffset] = static_cast<f32>(VisionMarker::ProbePoints_X[iOffset]) * fixedPointDivider;
        probeYOffsetsF32[iOffset] = static_cast<f32>(VisionMarker::ProbePoints_Y[iOffset]) * fixedPointDivider;
      }
      
      //u8 minValue = std::numeric_limits<u8>::max();
      //u8 maxValue = 0;
      
      for(s32 iProbe=0; iProbe<numProbes; ++iProbe)
      {
        const f32 xCenter = static_cast<f32>(VisionMarker::ProbeCenters_X[iProbe]) * fixedPointDivider;
        const f32 yCenter = static_cast<f32>(VisionMarker::ProbeCenters_Y[iProbe]) * fixedPointDivider;
        
        u32 accumulator = 0;
        for(s32 iOffset=0; iOffset<numProbeOffsets; iOffset++) {
          // 1. Map each probe to its warped locations
          const f32 x = xCenter + probeXOffsetsF32[iOffset];
          const f32 y = yCenter + probeYOffsetsF32[iOffset];
          
          const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);
          
          const f32 warpedXf = (h00 * x + h01 *y + h02) * homogenousDivisor;
          const f32 warpedYf = (h10 * x + h11 *y + h12) * homogenousDivisor;
          
          const s32 warpedX = Round<s32>(warpedXf);
          const s32 warpedY = Round<s32>(warpedYf);
          
          // 2. Sample the image
          
          // This should only fail if there's a bug in the quad extraction
          AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < image.get_size(0) && warpedX < image.get_size(1));
          
          const u8 imageValue = *image.Pointer(warpedY, warpedX);
          
          accumulator += imageValue;
        } // for each probe offset
        
        pProbeData[iProbe] = static_cast<u8>(accumulator / numProbeOffsets);
        
        /*
         // Keep track of min/max for normalization below
         if(pProbeData[iProbe] < minValue) {
         minValue = pProbeData[iProbe];
         } else if(pProbeData[iProbe] > maxValue) {
         maxValue = pProbeData[iProbe];
         }
         */
      } // for each probe
      
      /*
       // Normalization to scale between 0 and 255:
       AnkiConditionalErrorAndReturnValue(maxValue > minValue, RESULT_FAIL,
       "NearestNeighborLibrary.GetProbeValues",
       "Probe max (%d) <= min (%d)", maxValue, minValue);
       const s32 divisor = static_cast<s32>(maxValue - minValue);
       AnkiAssert(divisor > 0);
       for(s32 iProbe=0; iProbe<numProbes; ++iProbe) {
       pProbeData[iProbe] = (255*static_cast<s32>(pProbeData[iProbe] - minValue)) / divisor;
       }
       */
      
      // Illumination normalization code (assuming we are not using the same filtering used
      // by the fiducial detector to improve quad refinment precision)
      
      if(doIlluminationNormalization)
      {
        // TODO: Make this a member of the class and pre-allocate it?
        cv::Mat_<s16> _probeFiltering;
        
        cv::Mat_<u8> temp(VisionMarker::GRIDSIZE, VisionMarker::GRIDSIZE, probeValues.data);
        //cv::imshow("Original ProbeValues", temp);
        
        // TODO: Would it be faster to box filter and subtract it from the original?
        static cv::Mat_<s16> kernel;
        if(kernel.empty()) {
          assert(VisionMarker::GRIDSIZE % 2 == 0);
          const s32 halfSize = VisionMarker::GRIDSIZE/2;
          kernel = cv::Mat_<s16>(halfSize-1,halfSize-1);
          kernel = -1;
          kernel((halfSize-1)/2,(halfSize-1)/2) = kernel.rows*kernel.cols - 1;
        }
        cv::filter2D(temp, _probeFiltering, _probeFiltering.depth(), kernel,
                     cv::Point(-1,-1), 0, cv::BORDER_REPLICATE);
        //cv::imshow("ProbeFiltering", _probeFiltering);
        cv::normalize(_probeFiltering, temp, 255.f, 0.f, CV_MINMAX);
      }
      
      return RESULT_OK;
    } // VisionMarker::GetProbeValues()
    
    
    Vision::MarkerType VisionMarker::RemoveOrientation(Vision::MarkerType orientedMarker)
    {
#     if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR || \
         RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
      return RemoveOrientationLUT[orientedMarker];
#     elif RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
      return VisionMarkerDecisionTree::RemoveOrientationLUT[orientedMarker];
#     endif
    }

#if 0
    Result VisionMarker::ComputeBrightDarkValues(const Array <u8> &image,
                                                 const Array<f32> &homography, const f32 minContrastRatio,
                                                 f32& brightValue, f32& darkValue, bool& enoughContrast)
    {
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
      
      const f32 h00 = homography[0][0];
      const f32 h10 = homography[1][0];
      const f32 h20 = homography[2][0];
      const f32 h01 = homography[0][1];
      const f32 h11 = homography[1][1];
      const f32 h21 = homography[2][1];
      const f32 h02 = homography[0][2];
      const f32 h12 = homography[1][2];
      const f32 h22 = homography[2][2];
      
      static const Quadrilateral<f32> outsideQuad(Point<f32>(0,0), Point<f32>(0,1), Point<f32>(1,1), Point<f32>(1,0));
      static const Quadrilateral<f32> insideQuad(Point<f32>(0.2,0.2), Point<f32>(0.2,.8), Point<f32>(.8,.8), Point<f32>(0.8,0.2));
      
      static const Quadrilateral<f32>* quads[2] = {&outsideQuad, &insideQuad};
      static const u8 fillValues[2] = {1,0};
      
      cv::Mat_<u8> mask = cv::Mat_<u8>::zeros(imageHeight, imageWidth);
      
      for(s32 iQuad=0; iQuad<2; ++iQuad) {
        const Quadrilateral<f32>& quad = *(quads[iQuad]);
        cv::vector<cv::Point> cvQuad(4);
        
        for(s32 i=0; i<4; ++i) {
          
          const f32 x = quad[i].x;
          const f32 y = quad[i].y;
          
          const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);
          
          const f32 warpedXf = (h00 * x + h01 * y + h02) * homogenousDivisor;
          const f32 warpedYf = (h10 * x + h11 * y + h12) * homogenousDivisor;
          
          cvQuad[i].x = Round<s32>(warpedXf);
          cvQuad[i].y = Round<s32>(warpedYf);
          
        }
        cv::fillConvexPoly(mask, cvQuad, fillValues[iQuad]);
      }
      
      // DEBUG: Show mask
      //cv::imshow("Min/Max Mask", mask*255);
      //cv::waitKey(5);
      
      cv::Mat cvImage;
      ArrayToCvMat(image, &cvImage);
      double darkValueF64, brightValueF64;
      cv::minMaxLoc(cvImage, &darkValueF64, &brightValueF64, 0, 0, mask);
      
      darkValue   = static_cast<f32>(darkValueF64);
      brightValue = static_cast<f32>(brightValueF64);
      
      enoughContrast = (brightValue > minContrastRatio * darkValue);
      
      return RESULT_OK;
    }
#endif
    

    Result VisionMarker::ComputeBrightDarkValues(const Array <u8> &image, const f32 minContrastRatio, const bool isDarkOnLight,
      f32& backgroundValue, f32& foregroundValue, bool& enoughContrast)
    {
#     if RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
      using namespace VisionMarkerDecisionTree;
#     endif

      Result lastResult = RESULT_OK;

      Initialize();

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
#endif

      const f32 h00 = homography[0][0];
      const f32 h10 = homography[1][0];
      const f32 h20 = homography[2][0];
      const f32 h01 = homography[0][1];
      const f32 h11 = homography[1][1];
      const f32 h21 = homography[2][1];
      const f32 h02 = homography[0][2];
      const f32 h12 = homography[1][2];
      const f32 h22 = homography[2][2];

      f32 fixedPointDivider;

      AnkiAssert(VisionMarker::NUM_FRACTIONAL_BITS >= 0);

      fixedPointDivider = 1.0f / static_cast<f32>(1 << VisionMarker::NUM_FRACTIONAL_BITS);

      f32 probePointsX_F32[NUM_PROBE_POINTS];
      f32 probePointsY_F32[NUM_PROBE_POINTS];

      for(s32 i_pt=0; i_pt<NUM_PROBE_POINTS; i_pt++) {
        probePointsX_F32[i_pt] = static_cast<f32>(ProbePoints_X[i_pt]) * fixedPointDivider;
        probePointsY_F32[i_pt] = static_cast<f32>(ProbePoints_Y[i_pt]) * fixedPointDivider;
      }

#     if DEBUG_BRIGHT_DARK_PAIRS
      cv::Mat brightDarkPairsImage;
      ArrayToCvMat(image, &brightDarkPairsImage);
      cv::cvtColor(brightDarkPairsImage, brightDarkPairsImage, CV_GRAY2BGR);
      {
        static const Quadrilateral<f32> outsideQuad(Point<f32>(0,0), Point<f32>(0,1), Point<f32>(1,1), Point<f32>(1,0));
        static const Quadrilateral<f32> insideQuad(Point<f32>(0.2,0.2), Point<f32>(0.2,.8), Point<f32>(.8,.8), Point<f32>(0.8,0.2));
        static const Quadrilateral<f32> middleQuad(Point<f32>(0.1,0.1), Point<f32>(0.1,.9), Point<f32>(.9,.9), Point<f32>(0.9,0.1));
        static const Quadrilateral<f32>* quads[3] = {&outsideQuad, &insideQuad, &middleQuad};
        
        cv::Mat_<u8> mask = cv::Mat_<u8>::zeros(imageHeight, imageWidth);
        
        for(s32 iQuad=0; iQuad<3; ++iQuad) {
          const Quadrilateral<f32>& quad = *(quads[iQuad]);
          std::vector<cv::Point> cvQuad(4);
          
          for(s32 i=0; i<4; ++i) {
            
            const f32 x = quad[i].x;
            const f32 y = quad[i].y;
            
            const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);
            
            const f32 warpedXf = (h00 * x + h01 * y + h02) * homogenousDivisor;
            const f32 warpedYf = (h10 * x + h11 * y + h12) * homogenousDivisor;
            
            cvQuad[i].x = Round<s32>(warpedXf);
            cvQuad[i].y = Round<s32>(warpedYf);
            
          }
          cv::polylines(brightDarkPairsImage, cvQuad, true, cv::Scalar(0,0,255));
        }
      }
#     endif
      
      enoughContrast = true;

      const f32 divisor = 1.f / static_cast<f32>(NUM_PROBE_POINTS);

      u32 totalForegroundAccumulator = 0, totalBackgroundAccumulator = 0; // for all pairs
      for(s32 i_probe=0; i_probe<NUM_THRESHOLD_PROBES; ++i_probe) {
        const f32 xCenterDark = static_cast<f32>(ThresholdDarkProbe_X[i_probe]) * fixedPointDivider;
        const f32 yCenterDark = static_cast<f32>(ThresholdDarkProbe_Y[i_probe]) * fixedPointDivider;

        const f32 xCenterBright = static_cast<f32>(ThresholdBrightProbe_X[i_probe]) * fixedPointDivider;
        const f32 yCenterBright = static_cast<f32>(ThresholdBrightProbe_Y[i_probe]) * fixedPointDivider;

        u32 fgAccumulator = 0, bgAccumulator = 0; // for each bright/dark pair

        // TODO: Make getting the average value of a probe pattern into a function
        for(s32 i_pt=0; i_pt<NUM_PROBE_POINTS; i_pt++) {
          { // Dark points
            // 1. Map each probe to its warped locations
            const f32 x = xCenterDark + probePointsX_F32[i_pt];
            const f32 y = yCenterDark + probePointsY_F32[i_pt];

            const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);

            const f32 warpedXf = (h00 * x + h01 * y + h02) * homogenousDivisor;
            const f32 warpedYf = (h10 * x + h11 * y + h12) * homogenousDivisor;

            const s32 warpedX = Round<s32>(warpedXf);
            const s32 warpedY = Round<s32>(warpedYf);

            // 2. Sample the image

            // This should only fail if there's a bug in the quad extraction
            AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < imageHeight && warpedX < imageWidth);

            const u8 imageValue = *image.Pointer(warpedY, warpedX);

            fgAccumulator += imageValue;
            
#           if DEBUG_BRIGHT_DARK_PAIRS
            if(probePointsX_F32[i_pt]==0.f && probePointsY_F32[i_pt]==0.f) {
              cv::circle(brightDarkPairsImage, cv::Point(warpedX,warpedY), 2, cv::Scalar(0,255,255), -1);
            }
#           endif
          }

          { // Bright points
            // 1. Map each probe to its warped locations
            const f32 x = xCenterBright + probePointsX_F32[i_pt];
            const f32 y = yCenterBright + probePointsY_F32[i_pt];

            const f32 homogenousDivisor = 1.0f / (h20*x + h21*y + h22);

            const f32 warpedXf = (h00 * x + h01 * y + h02) * homogenousDivisor;
            const f32 warpedYf = (h10 * x + h11 * y + h12) * homogenousDivisor;

            const s32 warpedX = Round<s32>(warpedXf);
            const s32 warpedY = Round<s32>(warpedYf);

            // 2. Sample the image

            // This should only fail if there's a bug in the quad extraction
            AnkiAssert(warpedY >= 0  && warpedX >= 0 && warpedY < imageHeight && warpedX < imageWidth);

            const u8 imageValue = *image.Pointer(warpedY, warpedX);

            bgAccumulator += imageValue;
            
#           if DEBUG_BRIGHT_DARK_PAIRS
            if(probePointsX_F32[i_pt]==0.f && probePointsY_F32[i_pt]==0.f) {
              cv::circle(brightDarkPairsImage, cv::Point(warpedX,warpedY), 2, cv::Scalar(255,0,0), -1);
            }
#           endif
          }
        } // FOR each probe point

#       if DEBUG_BRIGHT_DARK_PAIRS
        cv::imshow("Bright/Dark Probes", brightDarkPairsImage);
        cv::waitKey(5);
#       endif
        
        backgroundValue = static_cast<f32>(bgAccumulator) * divisor;
        foregroundValue   = static_cast<f32>(fgAccumulator) * divisor;

        enoughContrast = (isDarkOnLight ?
                          backgroundValue > minContrastRatio * foregroundValue :
                          foregroundValue > minContrastRatio * backgroundValue);
        
        if(!enoughContrast) {
          // Something is wrong: not enough constrast at this bright/dark pair
          return RESULT_OK;
        }

        totalBackgroundAccumulator += bgAccumulator;
        totalForegroundAccumulator += fgAccumulator;
      } // FOR each probe

      const f32 totalDivisor = 1.f / static_cast<f32>(NUM_PROBE_POINTS * NUM_THRESHOLD_PROBES);
      backgroundValue = static_cast<f32>(totalBackgroundAccumulator) * totalDivisor;
      foregroundValue   = static_cast<f32>(totalForegroundAccumulator)   * totalDivisor;

      enoughContrast = (isDarkOnLight ?
                        backgroundValue > minContrastRatio * foregroundValue :
                        foregroundValue > minContrastRatio * backgroundValue);
      
      return lastResult;
    }

    Result VisionMarker::RefineCorners(const Array<u8> &image,
                                       const f32 minContrastRatio,
                                       const s32 refine_quadRefinementIterations,
                                       const s32 refine_numRefinementSamples,
                                       const f32 refine_quadRefinementMaxCornerChange,
                                       const f32 refine_quadRefinementMinCornerChange,
                                       const s32 quads_minQuadArea,
                                       const s32 quads_quadSymmetryThreshold,
                                       const s32 quads_minDistanceFromImageEdge,
                                       const Point<f32>& fiducialThicknessFraction,
                                       const Point<f32>& roundedCornersFraction,
                                       const bool isDarkOnLight,
                                       u8 &meanGrayvalueThreshold,
                                       MemoryStack scratch)
    {
      Result lastResult = RESULT_FAIL;

      // Store a copy of the initial homography in case we need to restore it later
      Array<f32> initHomography(3,3,scratch);
      initHomography.Set(homography);
      
      this->validity = UNKNOWN;

      BeginBenchmark("vmrc_brightdarkvals");
      f32 backgroundValue = 0.f, foregroundValue = 0.f;
      bool enoughContrast = false;
      if((lastResult = this->ComputeBrightDarkValues(image, minContrastRatio, isDarkOnLight, backgroundValue, foregroundValue, enoughContrast)) != RESULT_OK) {
        return lastResult;
      }
      EndBenchmark("vmrc_brightdarkvals");

      if(enoughContrast) {
        // If the contrast is sufficient, compute the threshold and parse the marker

        const Quadrilateral<f32> initQuad = this->corners;

        meanGrayvalueThreshold = static_cast<u8>(0.5f*(backgroundValue+foregroundValue));

        if(refine_quadRefinementIterations > 0) {
          BeginBenchmark("vmrc_quadrefine");

          if((lastResult = RefineQuadrilateral(
            initQuad,
            initHomography,
            image,
            fiducialThicknessFraction,
            roundedCornersFraction,
            refine_quadRefinementIterations,
            foregroundValue,
            backgroundValue,
            refine_numRefinementSamples,
            refine_quadRefinementMaxCornerChange,
            refine_quadRefinementMinCornerChange,
            this->corners,
            this->homography,
            scratch)) != RESULT_OK)
          {
            // TODO: Don't fail? Just warn and keep original quad?
            AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
              lastResult, "RefineQuadrilateral", "RefineQuadrilateral() failed with code %0x.", lastResult);
          }

          Quadrilateral<s16> refinedQuadS16;
          refinedQuadS16.SetCast(this->corners);

          bool areCornersDisordered;
          const bool isReasonable = IsQuadrilateralReasonable(refinedQuadS16, quads_minQuadArea, quads_quadSymmetryThreshold, quads_minDistanceFromImageEdge, image.get_size(0), image.get_size(1), areCornersDisordered);

          if(!isReasonable) {
            this->corners = initQuad;
          }

          EndBenchmark("vmrc_quadrefine");
        } else {
          // If we're not refining, the refined homography is the same as the initial one
          homography.Set(initHomography);
        }
      } else {
        // Not enough contrast at bright/dark pairs.
        this->validity = LOW_CONTRAST;

        // This is relatively common (and reasonable/expected), so maybe we
        // don't want to print these messages?
        /*
        #ifdef OUTPUT_FAILED_MARKER_STEPS
        AnkiWarn("VisionMarker::Extract", "Poor contrast marker detected");
        #endif
        */
      } // if contrast is sufficient

      return lastResult;
    } // RefineCorners()

    
    
    Result VisionMarker::Extract(const Array<u8> &image,
                                 const u8 grayvalueThreshold, // Trees: for thresholding probes, NN: max distance threshold
                                 const f32 minContrastRatio,
                                 MemoryStack scratch)
    {
#     if RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
      using namespace VisionMarkerDecisionTree;
#     endif

      Result lastResult = RESULT_FAIL;

      //AnkiConditionalErrorAndReturnValue(this->validity == VisionMarker::UNKNOWN,
      //  RESULT_FAIL, "VisionMarker::Extract", "Marker is not initialized. Perhaps RefineCorners() was not called, or Extract() was called twice.");

      const Quadrilateral<f32> initQuad = this->corners;

      Initialize();

      //s16 multiClassLabel = static_cast<s16>(MARKER_UNKNOWN);

      bool verified = false;
      OrientedMarkerLabel selectedLabel = MARKER_UNKNOWN;
      
      
#     if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR

      BeginBenchmark("vme_classify_nn");
      
#       if DO_RECOGNITION_TIMING
        const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
#       endif
      
      s32 label=-1;
      s32 distance = grayvalueThreshold;
      
      lastResult = VisionMarker::GetNearestNeighborLibrary().GetNearestNeighbor(image, homography, grayvalueThreshold, label, distance);
      
      AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                         "VisionMarker.Extract",
                                         "Failed finding nearest neighbor");
      
#       if DO_RECOGNITION_TIMING
        const std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
        const std::chrono::milliseconds nnTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        CoreTechPrint("GetNearestNeighbor() Time: %dms\n", nnTime);
#       endif
      
      if(label != -1) {
        if (distance < grayvalueThreshold) {
          selectedLabel = static_cast<OrientedMarkerLabel>(label);
          verified = (selectedLabel != MARKER_UNKNOWN &&
                      selectedLabel != MARKER_INVALID_000);
        } else {
          AnkiWarn("VisionMarker.Extract",
                   "Valid label returned as nearest neighbor, but distance (%d) >= threshold (%d)",
                   distance, grayvalueThreshold);
        }
      }
      
      EndBenchmark("vme_classify_nn");
      
      BeginBenchmark("vme_verify");
      
#     elif RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
      BeginBenchmark("vme_classify_tree");
      
      AnkiAssert(NUM_TREES <= std::numeric_limits<u8>::max());
      u8 predictedLabelsHist[NUM_MARKER_LABELS_ORIENTED];
      for(s32 iLabel=0; iLabel<NUM_MARKER_LABELS_ORIENTED; ++iLabel) {
        predictedLabelsHist[iLabel] = 0;
      }
      
      for(s32 iTree=0; iTree < NUM_TREES; ++iTree) {
        s32 tempLabel;
        if((lastResult = VisionMarker::multiClassTrees[iTree].Classify(image, homography, grayvalueThreshold, tempLabel)) != RESULT_OK) {
          return lastResult;
        }
        AnkiAssert(tempLabel < NUM_MARKER_LABELS_ORIENTED);
        AnkiAssert(tempLabel >= 0);
        ++predictedLabelsHist[tempLabel];
      }
      
      
      EndBenchmark("vme_classify_tree");
      
      BeginBenchmark("vme_verify");
      // See if a majority of the trees voted for the same label
      // TODO: use plurality of trees instead?
      
      u8 maxVotes = 0;
      for(s32 iLabel=0; iLabel<NUM_MARKER_LABELS_ORIENTED; ++iLabel) {
        if(predictedLabelsHist[iLabel] > maxVotes) {
          maxVotes = predictedLabelsHist[iLabel];
          selectedLabel = static_cast<OrientedMarkerLabel>(iLabel);
        }
      }
      
      AnkiConditionalErrorAndReturnValue(maxVotes>0, RESULT_FAIL, "VisionMarker.Extract.NoVotes", "No votes given to any marker label")
      AnkiConditionalErrorAndReturnValue(selectedLabel>=0, RESULT_FAIL, "VisionMarker.Extract.NoBestLabel", "No label with max votes selected");
      
      const f32 numVotesForMajority = static_cast<f32>(NUM_TREES)*0.5f;
      verified = (static_cast<f32>(maxVotes) > numVotesForMajority &&
                  selectedLabel != MARKER_INVALID_000 &&
                  selectedLabel != MARKER_UNKNOWN);
      
#     elif RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
      
      Vision::ConvolutionalNeuralNet& cnn = VisionMarker::GetCNN();
      
#       if DO_RECOGNITION_TIMING
        const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
#       endif
      
      std::string className;
      size_t classLabel;
      static cv::Mat_<u8> _probeValues(VisionMarker::GRIDSIZE, VisionMarker::GRIDSIZE);
      VisionMarker::GetProbeValues(image, homography, true, _probeValues);
      lastResult = cnn.Run(_probeValues, classLabel, className);
      
#       if DO_RECOGNITION_TIMING
        const std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
        const std::chrono::milliseconds nnTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        CoreTechPrint("CNN.Run() Time: %dms\n", nnTime);
#       endif
      
      AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                         "VisionMarker.Extract.CNNFail",
                                         "ConvolutionalNeuralNet.Run failed");
      
      verified = true;
      selectedLabel = static_cast<OrientedMarkerLabel>(classLabel);
      
#     endif // USE_NEAREST_NEIGHBOR_RECOGNITION
      
      if(verified)
      {
        // We have a valid, verified classification.
        
        // 1. Get the unoriented type
        this->markerType = RemoveOrientationLUT[selectedLabel];
        
#if 0
        // DEBUG!
        //         ||
        //   markerType == Vision::MARKER_LIGHTNINGBOLT_02)
        {
          printf("Seeing %s with Distance = %d\n", Vision::MarkerTypeStrings[markerType], distance);
          
        }
        
        cv::Mat_<u8> temp(32, 32, VisionMarker::GetNearestNeighborLibrary()._probeValues.data);
        if(markerType == Vision::MARKER_LIGHTNINGBOLTHOLLOW_05) {
          cv::imwrite("/Users/andrew/temp/wrongMarker.png", temp);
        } else if(markerType == Vision::MARKER_INVERTED_LIGHTNINGBOLT_02) {
          cv::imwrite("/Users/andrew/temp/rightMarker.png", temp);          
        }
#endif
        // 2. Reorder the original detected corners to the canonical ordering for
        // this type
        const Quadrilateral<f32> initQuad = this->corners;
        for(s32 i_corner=0; i_corner<4; ++i_corner)
        {
          this->corners[i_corner] = initQuad[CornerReorderLUT[selectedLabel][i_corner]];
        }
        
        // 3. Keep track of what the original orientation was
        this->observedOrientation = ObservedOrientationLUT[selectedLabel];
        
        // Mark this as a valid marker (note that reaching this point should
        // be the only way isValid is true.
        this->validity = VALID;
        
      } else {
        /* Disabling this since it's overly verbose now that we're using the
           the voting scheme and lots of things don't get majority vote.
#       ifdef OUTPUT_FAILED_MARKER_STEPS
        AnkiWarn("VisionMarker::Extract", "Verify failed or UNKNOWN/INVALID marker detected");
#       endif
         */
        this->validity = UNVERIFIED;
        this->markerType = Vision::MARKER_UNKNOWN;
        this->corners = initQuad; // Just copy the non-reordered corners
      }

      EndBenchmark("vme_verify");

      return lastResult;
    } // VisionMarker::Extract()

    Result VisionMarker::ExtractExhaustive(
      const VisionMarkerImages &allMarkerImages,
      const Array<u8> &image,
      MemoryStack fastScratch,
      MemoryStack slowScratch)
    {
      VisionMarker matchedMarker;
      f32 matchQuality;

      allMarkerImages.MatchExhaustive(image, this->corners, matchedMarker, matchQuality, fastScratch, slowScratch);

      return RESULT_OK;
    }

    s32 VisionMarker::get_serializationSize() const
    {
      // TODO: make the correct length
      return 96 + 10*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
    }

    Anki::Vision::MarkerType LookupMarkerType(const char * name)
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      const s32 MAX_NAME_LENGTH = 1024;
      std::size_t nameLength = strlen(name);

      // Remove the start "marker_" if it is present
      if(nameLength >= 7) {
        const char *markerString = "MARKER_";

        bool startIsMarker = true;
        for(s32 i=0; i<7; i++) {
          if(toupper(name[i]) != markerString[i]) {
            startIsMarker = false;
            break;
          }
        }

        if(startIsMarker) {
          name += 7;
          nameLength -= 7;
        }
      }

      // Remove anything before "/" or "\"
      s32 lastSlashLocation = -1;
      for(s32 i=0; i<nameLength; i++) {
        if(name[i] == '\\' || name[i] == '/') {
          lastSlashLocation = i;
        }
      }

      name += lastSlashLocation + 1;
      nameLength -= lastSlashLocation + 1;

      // Remove anything after a "."
      for(s32 i=0; i<nameLength; i++) {
        if(name[i] == '.') {
          nameLength = i;
          break;
        }
      }

      // If the name is too long, return invalid
      if(nameLength >= MAX_NAME_LENGTH) {
        return Anki::Vision::MARKER_UNKNOWN;
      }

      // Convert the input to upper case
      char upperCaseName[MAX_NAME_LENGTH];
      for(s32 i=0; i<nameLength; i++) {
        upperCaseName[i] = toupper(name[i]);
      }
      upperCaseName[nameLength] = '\0';

      for(s32 i=0; i<Anki::Vision::NUM_MARKER_TYPES; i++) {
        const char * withoutPrefixName = Anki::Vision::MarkerTypeStrings[i] + 7;
        if(strcmp(withoutPrefixName, upperCaseName) == 0) {
          return static_cast<Anki::Vision::MarkerType>(i);
        }
      }

#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV
      return Anki::Vision::MARKER_UNKNOWN;
    }

    VisionMarkerImages::VisionMarkerImages(const FixedLengthList<const char*> &imageFilenames, MemoryStack &memory)
      : isValid(false)
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      this->numDatabaseImages = imageFilenames.get_size();

      {
        const cv::Mat image = cv::imread(imageFilenames[0], CV_LOAD_IMAGE_UNCHANGED);
        this->databaseImageHeight = image.rows;
        this->databaseImageWidth = image.cols;

        AnkiConditionalErrorAndReturn(databaseImageWidth == databaseImageHeight,
          "VisionMarkerImages::VisionMarkerImages", "All images must be equal size and square");
      }

      /*images = FixedLengthList<Array<u8> >(numImages, memory, Flags::Buffer(false, false, true));*/
      databaseImages = Array<u8>(databaseImageHeight, databaseImageWidth*numDatabaseImages, memory, Flags::Buffer(true, false, true));
      databaseLabelIndexes = FixedLengthList<Anki::Vision::MarkerType>(numDatabaseImages, memory, Flags::Buffer(false, false, true));

      for(s32 iFile=0; iFile<numDatabaseImages; iFile++) {
        // Parse the image name, to fill in labels and indexes
        databaseLabelIndexes[iFile] = LookupMarkerType(imageFilenames[iFile]);

        // Load the image
        cv::Mat image = cv::imread(imageFilenames[iFile], CV_LOAD_IMAGE_UNCHANGED);

        AnkiConditionalErrorAndReturn(image.rows == databaseImageHeight && image.rows == databaseImageWidth,
          "VisionMarkerImages::VisionMarkerImages", "All images must be equal size and square");

        //Array<u8> imageArray(image.rows, image.cols, memory);

        for(s32 y=0; y<image.rows; y++) {
          const u8 * restrict pImage = image.data + y*image.step.buf[0];
          u8 * restrict pImageArray = databaseImages[y];

          if(image.step.buf[1] == 1) {
            AnkiAssert(false); // TODO: implement
          } else if(image.step.buf[1] == 3) {
            AnkiAssert(false); // TODO: implement
          } else if(image.step.buf[1] == 4) {
            for(s32 x=0; x<image.cols; x++) {
              const u8 b = pImage[4*x];
              const u8 g = pImage[4*x + 1];
              const u8 r = pImage[4*x + 2];
              const u8 a = pImage[4*x + 3];

              if(a < 128) {
                pImageArray[x*numDatabaseImages + iFile] = 255;
              } else {
                const s32 grayValue = (r + g + b) / 3;
                if(grayValue > 128) {
                  pImageArray[x*numDatabaseImages + iFile] = 255;
                } else {
                  pImageArray[x*numDatabaseImages + iFile] = 0;
                }
              }
            }
          }
        } // for(s32 y=0; y<image.rows; y++)
      } // for(s32 iFile=0; iFile<numDatabaseImages; iFile++)

#else
      AnkiError("VisionMarkerImages::VisionMarkerImages", "OpenCV is required to load files");
#endif // #if ANKICORETECH_EMBEDDED_USE_OPENCV ... #else

      this->isValid = true;
    }

    VisionMarkerImages::VisionMarkerImages(const s32 numDatabaseImages, const s32 databaseImageHeight, const s32 databaseImageWidth, u8 * pDatabaseImages, Anki::Vision::MarkerType * pDatabaseLabelIndexes)
      : numDatabaseImages(numDatabaseImages), databaseImageHeight(databaseImageHeight), databaseImageWidth(databaseImageWidth), isValid(true)
    {
      // Note the 2* lie about the size of the buffer
      this->databaseImages = Array<u8>(databaseImageHeight, numDatabaseImages*databaseImageWidth, pDatabaseImages, 2*databaseImageHeight*numDatabaseImages*databaseImageWidth, Flags::Buffer(false, false, true));
      this->databaseLabelIndexes = FixedLengthList<Anki::Vision::MarkerType>(numDatabaseImages, pDatabaseLabelIndexes, 2*numDatabaseImages*sizeof(Anki::Vision::MarkerType), Flags::Buffer(false, false, true));
    }

    Result VisionMarkerImages::Show(const s32 pauseMilliseconds) const
    {
#if ANKICORETECH_EMBEDDED_USE_OPENCV
      // TODO: implement for striped

      //for(s32 i=0; i<databaseImages.get_size(); i++) {
      //  //snprintf(name, 128, "Number %d", databaseLabelIndexes[i]);

      //  databaseImages[i].Show("Fiducial", false, false, false);
      //  cv::waitKey(pauseMilliseconds);
      //}
#endif

      return RESULT_OK;
    }

    Result VisionMarkerImages::MatchExhaustive(const Array<u8> &image, const Quadrilateral<f32> &quad, VisionMarker &extractedMarker, f32 &matchQuality, MemoryStack fastScratch, MemoryStack slowScratch) const
    {
      const s32 yIncrement = 1;
      const s32 xIncrement = 1;

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);

      AnkiAssert(databaseImageWidth == databaseImageHeight);

      AnkiConditionalErrorAndReturnValue(NotAliased(fastScratch, slowScratch),
        RESULT_FAIL_ALIASED_MEMORY, "VisionMarkerImages::MatchExhaustive", "fastScratch and slowScratch must be different");

      // 1. Compute the transformation from the quad to the known marker images
      //allImagesCorners = [0 0 allImageSize(2) allImageSize(2); 0 allImageSize(1) 0 allImageSize(1)]';
      Quadrilateral<f32> databaseImagesCorners(
        Point<f32>(0.0f,0.0f),
        Point<f32>(0.0f,static_cast<f32>(databaseImageHeight)),
        Point<f32>(static_cast<f32>(databaseImageWidth),0.0f),
        Point<f32>(static_cast<f32>(databaseImageWidth),static_cast<f32>(databaseImageHeight)));

      Array<f32> homography(3, 3, fastScratch);

      AnkiConditionalErrorAndReturnValue(homography.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "VisionMarkerImages::MatchExhaustive", "Out of memory");

      bool numericalFailure;
      Transformations::ComputeHomographyFromQuads(quad, databaseImagesCorners, homography, numericalFailure, fastScratch);

      // 2. For each pixel in the quad in the input image, compute the mean-absolute difference with each known marker image
      // Based off DrawFilledConvexQuadrilateral()

      const Rectangle<f32> boundingRect = quad.ComputeBoundingRectangle<f32>();
      const Quadrilateral<f32> sortedQuad = quad.ComputeClockwiseCorners<f32>();

      const f32 rect_y0 = boundingRect.top;
      const f32 rect_y1 = boundingRect.bottom;

      // For circular indexing
      Point<f32> corners[5];
      for(s32 i=0; i<4; i++) {
        corners[i] = sortedQuad[i];
      }
      corners[4] = sortedQuad[0];

      const s32 minYS32 = MAX(0,             Round<s32>(ceilf(rect_y0 - 0.5f)));
      const s32 maxYS32 = MIN(imageHeight-1, Round<s32>(floorf(rect_y1 - 0.5f)));
      const f32 minYF32 = minYS32 + 0.5f;
      const f32 maxYF32 = maxYS32 + 0.5f;
      const LinearSequence<f32> ys(minYF32, maxYF32);
      const s32 numYs = ys.get_size();

      const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
      const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
      const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; //const f32 h22 = 1.0f;

      const IntegerCounts imageCounts(image, quad, 1, 1, fastScratch);
      IntegerCounts::Statistics imageCountsStatistics = imageCounts.ComputeStatistics();
      const u8 imageThreshold = Round<s32>(imageCountsStatistics.mean);

#ifdef SHOW_EXHAUSTIVE_STEPS
      Array<u8> originalImageBinarized(imageHeight, imageWidth, slowScratch);
      originalImageBinarized.Set(64);

      Array<Array<u8>> toShowImages(4, numDatabaseImages, slowScratch);
      AnkiConditionalErrorAndReturnValue(toShowImages.IsValid(),
        RESULT_FAIL_OUT_OF_MEMORY, "VisionMarkerImages::MatchExhaustive", "Out of memory");

      for(s32 iRotation=0; iRotation<4; iRotation++) {
        for(s32 iDatabase=0; iDatabase<numDatabaseImages; iDatabase++) {
          toShowImages[iRotation][iDatabase] = Array<u8>(imageHeight, imageWidth, slowScratch);

          AnkiConditionalErrorAndReturnValue(toShowImages[iRotation][iDatabase].IsValid(),
            RESULT_FAIL_OUT_OF_MEMORY, "VisionMarkerImages::MatchExhaustive", "Out of memory");

          toShowImages[iRotation][iDatabase].Set(64);
        }
      }
#endif

      // Four rotations for each image
      s32 numBytesAllocated;
      s32 * totalDifferences = reinterpret_cast<s32*>(fastScratch.Allocate(4*sizeof(s32)*numDatabaseImages,true,numBytesAllocated));
      s32 numInBounds = 0;

      AnkiAssert(totalDifferences != NULL);

      //const u8 * restrict * restrict pDatabaseImages = reinterpret_cast<const u8**>(fastScratch.Allocate(sizeof(u8*)*numDatabaseImages,true,numBytesAllocated));
      //u8 ** pDatabaseImages = reinterpret_cast<u8**>(fastScratch.Allocate(sizeof(u8*)*numDatabaseImages,true,numBytesAllocated));

      // TODO: is memory being overwritten!!!??

      //AnkiAssert(pDatabaseImages != NULL);

      //for(s32 iDatabase=0; iDatabase<numDatabaseImages; iDatabase++) {
      //  pDatabaseImages[iDatabase] = this->databaseImages[iDatabase].Pointer(0,0);
      //}

      const u8 * restrict pDatabaseImages_start = this->databaseImages.Pointer(0,0);

      f32 yF32 = ys.get_start();
      for(s32 iy=0; iy<numYs; iy+=yIncrement) {
        // Compute all intersections
        f32 minXF32 = FLT_MAX;
        f32 maxXF32 = FLT_MIN;
        for(s32 iCorner=0; iCorner<4; iCorner++) {
          if( (corners[iCorner].y < yF32 && corners[iCorner+1].y >= yF32) || (corners[iCorner+1].y < yF32 && corners[iCorner].y >= yF32) ) {
            const f32 dy = corners[iCorner+1].y - corners[iCorner].y;
            const f32 dx = corners[iCorner+1].x - corners[iCorner].x;

            const f32 alpha = (yF32 - corners[iCorner].y) / dy;

            const f32 xIntercept = corners[iCorner].x + alpha * dx;

            minXF32 = MIN(minXF32, xIntercept);
            maxXF32 = MAX(maxXF32, xIntercept);
          }
        } // for(s32 iCorner=0; iCorner<4; iCorner++)

        const s32 minXS32 = MAX(0,            Round<s32>(floorf(minXF32+0.5f)));
        const s32 maxXS32 = MIN(imageWidth-1, Round<s32>(floorf(maxXF32-0.5f)));

        const s32 yS32 = minYS32 + iy;
        const u8 * restrict pImage = image.Pointer(yS32, 0);
        for(s32 x=minXS32; x<=maxXS32; x+=xIncrement) {
          // Do nearest-neighbor lookup from the query image to the image in the dataset

          const f32 yOriginal = yF32;
          const f32 xOriginal = static_cast<f32>(x);

          // TODO: These two could be strength reduced
          const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
          const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

          const f32 normalization = h20*xOriginal + h21*yOriginal + 1.0f;

          const s32 xTransformed0 = Round<s32>(xTransformedRaw / normalization);
          const s32 yTransformed0 = Round<s32>(yTransformedRaw / normalization);

          // If out of bounds, continue
          if(xTransformed0 < 0 || xTransformed0 >= databaseImageWidth || yTransformed0 < 0 || yTransformed0 >= databaseImageWidth) {
            continue;
          }

          const s32 curImageValue = (pImage[x] > imageThreshold) ? 255 : 0;

          const s32 xTransformed90 = databaseImageWidth - yTransformed0 - 1;
          const s32 yTransformed90 = xTransformed0;

          const s32 xTransformed180 = databaseImageWidth - xTransformed0 - 1;
          const s32 yTransformed180 = databaseImageHeight - yTransformed0 - 1;

          const s32 xTransformed270 = yTransformed0;
          const s32 yTransformed270 = databaseImageHeight - xTransformed0 - 1;

          numInBounds++;

          const s32 xTransformed[4] = {xTransformed0, xTransformed90, xTransformed180, xTransformed270};
          const s32 yTransformed[4] = {yTransformed0, yTransformed90, yTransformed180, yTransformed270};

          // The four rotation (0, 90, 180, 270)
          //   0: x = x         and y = y
          //  90: x = width-y-1 and y = x
          // 180: x = width-x-1 and y = height-y-1
          // 270: x = y         and y = height-x-1

#ifdef SHOW_EXHAUSTIVE_STEPS
          originalImageBinarized[yS32][x] = curImageValue;
#endif

          for(s32 iRotation=0; iRotation<4; iRotation++) {
            const u8 * restrict pDatabaseImages = pDatabaseImages_start + numDatabaseImages*(yTransformed[iRotation]*databaseImageWidth + xTransformed[iRotation]);

            for(s32 iDatabase=0; iDatabase<numDatabaseImages; iDatabase++) {
              const s32 databaseImageValue = pDatabaseImages[iDatabase];
              totalDifferences[numDatabaseImages*iRotation + iDatabase] += ABS(curImageValue - databaseImageValue);
#ifdef SHOW_EXHAUSTIVE_STEPS
              toShowImages[iRotation][iDatabase][yS32][x] = databaseImageValue;
#endif
            } // for(s32 iDatabase=0; iDatabase<numDatabaseImages; iDatabase++)
          } // for(s32 iRotation=0; iRotation<4; iRotation++)
        } // for(s32 x=minXS32; x<=maxXS32; x+=xIncrement)

        yF32 += static_cast<f32>(yIncrement);
      } // for(s32 iy=0; iy<numYs; iy++)

      // TODO: best difference among the means, and store it

      s32 bestDatabaseImage = -1;
      s32 bestDatabaseRotation = -1;
      s32 bestDatabaseDifference = std::numeric_limits<s32>::max();

#ifdef SHOW_EXHAUSTIVE_STEPS
      image.Show("image", false, false, true);
      originalImageBinarized.Show("originalImageBinarized", true, false, true);
#endif

      for(s32 iRotation=0; iRotation<4; iRotation++) {
        //for(s32 iRotation=0; iRotation<1; iRotation++) {
        for(s32 iDatabase=0; iDatabase<numDatabaseImages; iDatabase++) {
          //for(s32 iDatabase=60; iDatabase<90; iDatabase++) {
          const s32 curTotalDifference = totalDifferences[numDatabaseImages*iRotation + iDatabase];

          //CoreTechPrint("(%d,%d) curTotalDifference = %f\n", iRotation, iDatabase, curTotalDifference / (255.0f * static_cast<f32>(numInBounds)));

#ifdef SHOW_EXHAUSTIVE_STEPS
          toShowImages[iRotation][iDatabase].Show("database", false, false, true);
          cv::waitKey();
#endif

          if(curTotalDifference < bestDatabaseDifference) {
            bestDatabaseDifference = curTotalDifference;
            bestDatabaseRotation = iRotation;
            bestDatabaseImage = iDatabase;
          }
        }
      } // for(s32 iDatabase=0; iDatabase<numDatabaseImages; iDatabase++)

      extractedMarker = VisionMarker(quad, VisionMarker::VALID);
      extractedMarker.markerType = this->databaseLabelIndexes[bestDatabaseImage];

      if(bestDatabaseRotation == 0) {
        extractedMarker.observedOrientation = 0.0f;
      } else if(bestDatabaseRotation == 1) {
        extractedMarker.observedOrientation = 90.0f;
      } else if(bestDatabaseRotation == 2) {
        extractedMarker.observedOrientation = 180.0f;
      } else if(bestDatabaseRotation == 3) {
        extractedMarker.observedOrientation = 270.0f;
      }

      matchQuality = bestDatabaseDifference / (255.0f * static_cast<f32>(numInBounds));

      // Check if there was any nonsense in the loop
      AnkiAssert(fastScratch.IsValid());

      return RESULT_OK;
    }

    bool VisionMarkerImages::IsValid() const
    {
      if(!databaseImages.IsValid())
        return false;

      if(!databaseLabelIndexes.IsValid())
        return false;

      return this->isValid;
    }

    s32 VisionMarkerImages::get_numDatabaseImages() const
    {
      return numDatabaseImages;
    }

    s32 VisionMarkerImages::get_databaseImageHeight() const
    {
      return databaseImageHeight;
    }

    s32 VisionMarkerImages::get_databaseImageWidth() const
    {
      return databaseImageWidth;
    }

    const Array<u8>& VisionMarkerImages::get_databaseImages() const
    {
      return databaseImages;
    }

    const FixedLengthList<Anki::Vision::MarkerType>& VisionMarkerImages::get_databaseLabelIndexes()
    {
      return databaseLabelIndexes;
    }
  } // namespace Embedded
} // namespace Anki
