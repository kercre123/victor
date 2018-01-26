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

#include "coretech/common/robot/array2d.h"
#include "coretech/common/robot/geometry.h"

#include "coretech/vision/robot/connectedComponents.h"

#include "coretech/vision/shared/MarkerCodeDefinitions.h"

// For old QR-style BlockMarkers
#define MAX_FIDUCIAL_MARKER_BITS 25
#define MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS 81

// Available marker recognition methods
#define RECOGNITION_METHOD_DECISION_TREES   0
#define RECOGNITION_METHOD_NEAREST_NEIGHBOR 1
#define RECOGNITION_METHOD_CNN              2

// Choose the recognition method here to be one from the above list
#define RECOGNITION_METHOD RECOGNITION_METHOD_NEAREST_NEIGHBOR

// Settings/includes required by the various methods
#if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR
#  include "coretech/vision/robot/nearestNeighborLibrary.h"
#  include "coretech/vision/engine/convolutionalNeuralNet.h"

#elif RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
#  include "coretech/vision/robot/decisionTree_vision.h"
#  include "coretech/vision/robot/visionMarkerDecisionTrees.h"
   // Set to 1 to use two "red/black" verification trees
   // Set to 0 to use (older) one-vs-all verifiers for each class
#  define USE_RED_BLACK_VERIFICATION_TREES 1

#elif RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
#  include "coretech/vision/engine/convolutionalNeuralNet.h"

#else
#  error Invalid RECOGNITION_METHOD set!
#endif

#define NUM_BYTES_probeLocationsBuffer (sizeof(Point<s16>)*MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS + MEMORY_ALIGNMENT + 32)
#define NUM_BYTES_probeWeightsBuffer (sizeof(s16)*MAX_FIDUCIAL_MARKER_BIT_PROBE_LOCATIONS  + MEMORY_ALIGNMENT + 32)
#define NUM_BYTES_bitsBuffer (sizeof(FiducialMarkerParserBit)*MAX_FIDUCIAL_MARKER_BITS + MEMORY_ALIGNMENT + 32)

namespace Anki
{ 
  namespace Embedded
  {
    class VisionMarker;

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

    class VisionMarkerImages
    {
    public:
      // Load the images from disk (requires OpenCV)
      VisionMarkerImages(const FixedLengthList<const char*> &imageFilenames, MemoryStack &memory);

      // Makes a shallow copy, based on the pointers
      VisionMarkerImages(const s32 numDatabaseImages, const s32 databaseImageHeight, const s32 databaseImageWidth, u8 * pDatabaseImages, Anki::Vision::MarkerType * pDatabaseLabelIndexes);

      Result Show(const s32 pauseMilliseconds) const;

      Result MatchExhaustive(const Array<u8> &image, const Quadrilateral<f32> &quad, VisionMarker &extractedMarker, f32 &matchQuality, MemoryStack fastScratch, MemoryStack slowScratch) const;

      bool IsValid() const;

      s32 get_numDatabaseImages() const;

      s32 get_databaseImageHeight() const;

      s32 get_databaseImageWidth() const;

      const Array<u8>& get_databaseImages() const;

      const FixedLengthList<Anki::Vision::MarkerType>& get_databaseLabelIndexes();

    protected:
      s32 numDatabaseImages;
      s32 databaseImageHeight;
      s32 databaseImageWidth;

      Array<u8> databaseImages; //< Striped, so for a given pixel, all images data is consecutive. (e.g. the first N bytes are the pixel (0,0) for all images)

      FixedLengthList<Anki::Vision::MarkerType> databaseLabelIndexes; // (aka name, aka codeName)

      bool isValid;
    };

    // A VisionMarker is a location Quadrilateral, with a markerType.
    class VisionMarker
    {
    public:

      enum ValidityCode {
        VALID = 0,
        VALID_BUT_NOT_DECODED,
        LOW_CONTRAST,
        UNVERIFIED,
        UNKNOWN,
        WEIRD_SHAPE,
        NUMERICAL_FAILURE,
        REFINEMENT_FAILURE
      };

      //Quadrilateral<s16> corners; // SQ 15.0 (Though may be changed later)
      Quadrilateral<f32> corners;
      Vision::MarkerType markerType;
      f32 observedOrientation; //< In degrees. TODO: change to radians or discrete
      ValidityCode validity;
      Array<f32> homography;
      
      // All the points on the fiducial used for quad refinement
      VisionMarker();
      VisionMarker(const Quadrilateral<s16> &corners, const ValidityCode validity);
      VisionMarker(const Quadrilateral<f32> &corners, const ValidityCode validity);

      Result RefineCorners(const Array<u8> &image,
                           const f32 minContrastRatio,
                           const s32 refine_quadRefinementIterations,
                           const s32 refine_numRefinementSamples,
                           const f32 refine_quadRefinementMaxCornerChange,
                           const f32 refine_quadRefinementMinCornerChange,
                           const s32 quads_minQuadArea,
                           const s32 quads_quadSymmetryThreshold,
                           const s32 quads_minDistanceFromImageEdge,
                           const Point<f32>& fiducialThicknessFraction,
                           const Point<f32>& roundedCornerFraction,
                           const bool isDarkOnLight,
                           u8 &meanGrayvalueThreshold, //< Computed for Extract()
                           MemoryStack scratch);

      Result Extract(const Array<u8> &image,
                     const u8 meanGrayvalueThreshold, //< Computed by RefineCorners()
                     const f32 minContrastRatio,
                     MemoryStack scratch);

      Result ExtractExhaustive(const VisionMarkerImages &allMarkerImages,
                               const Array<u8> &image,
                               MemoryStack fastScratch,
                               MemoryStack slowScratch);

      void Print() const;

      Result Serialize(const char *objectName, SerializedBuffer &buffer) const;
      Result SerializeRaw(const char *objectName, void ** buffer, s32 &bufferLength) const; // Updates the buffer pointer and length before returning
      Result Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack scratch); // Updates the buffer pointer and length before returning

      s32 get_serializationSize() const;

      static Vision::MarkerType RemoveOrientation(Vision::MarkerType orientedMarker);
      
      // Output probevalues should already have NUM_PROBES elements in it
      static Result GetProbeValues(const Array<u8> &image,
                                   const Array<f32> &homography,
                                   const bool doIlluminatioNormalization,
                                   cv::Mat_<u8> &probeValues);

      static const s32 NUM_FRACTIONAL_BITS;
      
      static const u32 NUM_PROBE_POINTS;
      static const s16 ProbePoints_X[];
      static const s16 ProbePoints_Y[];
      
      static const u32 NUM_THRESHOLD_PROBES;
      static const s16 ThresholdDarkProbe_X[];
      static const s16 ThresholdDarkProbe_Y[];
      static const s16 ThresholdBrightProbe_X[];
      static const s16 ThresholdBrightProbe_Y[];
      
      static const u32 GRIDSIZE;
      static const u32 NUM_PROBES;
      static const s16 ProbeCenters_X[];
      static const s16 ProbeCenters_Y[];

      static void SetDataPath(const std::string& dataPath);
      
#     if RECOGNITION_METHOD == RECOGNITION_METHOD_NEAREST_NEIGHBOR
      static NearestNeighborLibrary& GetNearestNeighborLibrary();
#     endif
      
    protected:
      // The constructor isn't always called, so initialize has to be checked in multiple places
      // TODO: make less hacky
      void Initialize();

      static std::string _dataPath;
      
      //Result ComputeThreshold(const Array <u8> &image, const Array<f32> &homography,
      //  const f32 minContrastRatio, bool &isHighContrast, u8 &meanGrayvalueThreshold);

      Result ComputeBrightDarkValues(const Array <u8> &image, const f32 minContrastRatio, const bool isDarkOnLight,
                                     f32& backgroundValue, f32& foregroundValue,
                                     bool& enoughContrast);

#     if RECOGNITION_METHOD == RECOGNITION_METHOD_DECISION_TREES
      static bool areTreesInitialized;
      static FiducialMarkerDecisionTree multiClassTrees[VisionMarkerDecisionTree::NUM_TREES];
#     elif RECOGNITION_METHOD == RECOGNITION_METHOD_CNN
      static Vision::ConvolutionalNeuralNet& GetCNN();
      //cv::Mat_<u8> _probeValues;
#     endif
      
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

    // Ignores case, and ignore any "MARKER_" prefix
    // Also ignores anything before a "/" or "\", and anything after a ".", so you can pass in a filename
    Anki::Vision::MarkerType LookupMarkerType(const char * name);
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_FIDUCIAL_MARKER_H_
