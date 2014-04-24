/**
File: fiducialMarkers.h
Author: Peter Barnum
Created: 2013

Final step for detection and parsing of a fiducial marker.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
#define _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/geometry.h"

#include "anki/vision/robot/connectedComponents.h"

#include "anki/vision/robot/decisionTree_vision.h"

#include "anki/vision/robot/visionMarkerDecisionTrees.h"

// For old QR-style BlockMarkers
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

    // A VisionMarker is a location Quadrilateral, with a markerType.
    class VisionMarker
    {
    public:

      Quadrilateral<s16> corners; // SQ 15.0 (Though may be changed later)
      Vision::MarkerType markerType;
      f32 observedOrientation; //< In degrees. TODO: change to radians or discrete
      bool isValid;

      VisionMarker();

      Result Extract(const Array<u8> &image, const Quadrilateral<s16> &quad,
        const Array<f32> &homography, const f32 minContrastRatio);

      void Print() const;

      Result Serialize(const char *objectName, SerializedBuffer &buffer) const;
      Result SerializeRaw(const char *objectName, void ** buffer, s32 &bufferLength) const; // Updates the buffer pointer and length before returning
      Result Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack scratch); // Updates the buffer pointer and length before returning

      s32 get_serializationSize() const;

    protected:
      // The constructor isn't always called, so initialize has to be checked in multiple places
      // TODO: make less hacky
      void Initialize();

      Result ComputeThreshold(const Array <u8> &image, const Array<f32> &homography,
        const f32 minContrastRatio, bool &isHighContrast, u8 &meanGrayvalueThreshold);

      static bool areTreesInitialized;
      static FiducialMarkerDecisionTree multiClassTree;
      static FiducialMarkerDecisionTree verificationTrees[VisionMarkerDecisionTree::NUM_MARKER_LABELS_ORIENTED];
    }; // class VisionMarker

    // A FiducialMarkerParserBit object samples an input image, to determine if a given image
    // area is binary 0 or 1.
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

      FiducialMarkerParserBit(MemoryStack &memory);

      FiducialMarkerParserBit(const FiducialMarkerParserBit& bit2);

      // All data from probeLocations is copied into this instance's local memory
      FiducialMarkerParserBit(const s16 * const probesX, const s16 * const probesY, const s16 * const probeWeights, const s32 numProbes, const FiducialMarkerParserBit::Type type, const s32 numFractionalBits, MemoryStack &memory);

      // Based on the probe locations that this object was initialized with, compute the mean value in a given image, with
      // an overall bounding Quadrilateral.
      //
      // NOTE:
      // The meanValue is not the mean value of the whole quad, just the part of the quad that is sampled by this Bit.
      Result ExtractMeanValue(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f32> &homography, s16 &meanValue) const;

      FiducialMarkerParserBit& operator= (const FiducialMarkerParserBit& bit2);

      const FixedLengthList<Point<s16> >& get_probeLocations() const;

      const FixedLengthList<s16>& get_probeWeights() const;

      FiducialMarkerParserBit::Type get_type() const;

      s32 get_numFractionalBits() const;

    protected:
      FixedLengthList<Point<s16> > probeLocations; //< A list of length MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS
      FixedLengthList<s16> probeWeights;
      FiducialMarkerParserBit::Type type;
      s32 numFractionalBits;
    }; // class FiducialMarkerParserBit

    // A FiducialMarkerParser takes an input image and Quadrilateral, and extracts the binary code.
    // It contains a list of FiducialMarkerParserBit objects that sample different locations of the code.
    class FiducialMarkerParser
    {
    public:

      // Initialize with the default grid type, converted from Matlab
      FiducialMarkerParser(MemoryStack &memory);

      FiducialMarkerParser(const FiducialMarkerParser& marker2);

      Result ExtractBlockMarker(const Array<u8> &image, const Quadrilateral<s16> &quad, const Array<f32> &homography, const f32 minContrastRatio, BlockMarker &marker, MemoryStack scratch) const;

      FiducialMarkerParser& operator= (const FiducialMarkerParser& marker2);

    protected:
      FixedLengthList<FiducialMarkerParserBit> bits;

      // The indexes of the four orientation bits
      s32 upBitIndex;
      s32 downBitIndex;
      s32 leftBitIndex;
      s32 rightBitIndex;

      // Once all FiducialMarkerParserBit have been parsed, determine if the code is valid, by checking its checksum
      static bool IsChecksumValid(const FixedLengthList<u8> &checksumBits, const FixedLengthList<u8> &blockBits, const FixedLengthList<u8> &faceBits);

      Result InitializeAsDefaultParser(MemoryStack &memory);

      // Once all FiducialMarkerParserBit have been parsed, determine the orientation of the code
      Result DetermineOrientationAndBinarizeAndReorderCorners(const FixedLengthList<s16> &meanValues, const f32 minContrastRatio, BlockMarker &marker, FixedLengthList<u8> &binarizedBits, MemoryStack scratch) const;

      // Once all FiducialMarkerParserBit have been parsed, determine the ID number of the code
      Result DecodeId(const FixedLengthList<u8> &binarizedBits, s16 &blockType, s16 &faceType, MemoryStack scratch) const;

      // Starting at startIndex, search through this->bits to find the first instance of the given type
      // Returns -1 if the type wasn't found
      s32 FindFirstBitOfType(const FiducialMarkerParserBit::Type type, const s32 startIndex) const;
    }; // class FiducialMarkerParser
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
