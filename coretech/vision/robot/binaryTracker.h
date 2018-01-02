/**
File: binaryTracker.h
Author: Peter Barnum
Created: 2014-02-21

Template tracking with a binary template

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_BINARY_TRACKER_H_
#define _ANKICORETECHEMBEDDED_VISION_BINARY_TRACKER_H_

#include "coretech/common/shared/types.h"
#include "coretech/vision/robot/edgeDetection.h"
#include "coretech/vision/robot/transformations.h"
#include "coretech/vision/robot/histogram.h"
#include "coretech/vision/shared/MarkerCodeDefinitions.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      class BinaryTracker
      {
        // A binary-image tracker. This is liable to be much faster than trackers like the standard
        // LucasKanadeTracker_Slow or LucasKanadeTracker_Affine, but is also liable to be less
        // accurate and more jittery.

      public:
        enum EdgeDetectionType {
          EDGE_TYPE_GRAYVALUE,
          EDGE_TYPE_DERIVATIVE,
        };

        typedef struct EdgeDetectionParameters {
          EdgeDetectionType type;

          // For grayvalue threshold computation
          s32 threshold_yIncrement; //< How many pixels to use in the y direction (4 is a good value?)
          s32 threshold_xIncrement; //< How many pixels to use in the x direction (4 is a good value?)
          f32 threshold_blackPercentile; //< What percentile of histogram energy is black? (.1 is a good value)
          f32 threshold_whitePercentile; //< What percentile of histogram energy is white? (.9 is a good value)
          f32 threshold_scaleRegionPercent; //< How much to scale template bounding box (.8 is a good value)

          // For grayvalue binarization
          s32 minComponentWidth; //< The smallest horizontal size of a component (1 to 4 is good)
          s32 maxDetectionsPerType; //< As many as you have memory and time for (500 is good for a small template, 2500 for a whole qvga image)

          // For derivative binarization
          s32 combHalfWidth; //< How far apart to compute the derivative difference (1 is good)
          s32 combResponseThreshold; //< The minimum absolute-value response to start an edge component (20 is good)

          s32 everyNLines; //< As many as you have time for

          EdgeDetectionParameters();
          EdgeDetectionParameters(EdgeDetectionType type, s32 threshold_yIncrement, s32 threshold_xIncrement, f32 threshold_blackPercentile, f32 threshold_whitePercentile, f32 threshold_scaleRegionPercent, s32 minComponentWidth, s32 maxDetectionsPerType, s32 combHalfWidth, s32 combResponseThreshold, s32 everyNLines);
        } EdgeDetectionParameters;

        BinaryTracker();

        // the real max number of edge pixels is maxEdgePixelsPerType*4, for each of the four edge types
        BinaryTracker(
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const EdgeDetectionParameters &edgeDetectionParams,
          MemoryStack &fastMemory,
          MemoryStack &slowMemory);

        // Similar to the other constructor, but computes the actual template edges from a reference binary image from a header file if one is available
        // NOTE: All the edgeDetection_threshold_* parameters are not for template extraction. They are just for computing the grayvalue for the next extraction
        BinaryTracker(
          const Anki::Vision::MarkerType markerType,
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent,
          const EdgeDetectionParameters &edgeDetectionParams,
          MemoryStack &fastMemory,
          MemoryStack &slowMemory);

        // Runs one iteration each of translation and projective
        //
        // numMatches returns the number of template pixels within verify_maxTranslationDistance of a point in nextImage
        // To check is the update is reasonable, numMatches / this->get_numTemplatePixels() will give the percentage of matches
        Result UpdateTrack_Normal(
          const Array<u8> &nextImage,
          const EdgeDetectionParameters &edgeDetectionParams,
          const s32 matching_maxTranslationDistance, const s32 matching_maxProjectiveDistance,
          const s32 verify_maxTranslationDistance, //< How close does a template pixel have to be to an edge to count as a match?
          const u8 verify_maxPixelDifference, //< See verify_numSimilarPixels
          const s32 verify_coordinateIncrement, //< Check every nth row and column (1 is the minimum, 2 is 4x faster, 3 is 9x faster, etc).
          s32 &numMatches,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        // WARNING: using a list is liable to be slower than normal, and not be more accurate
        Result UpdateTrack_List(
          const Array<u8> &nextImage,
          const EdgeDetectionParameters &edgeDetectionParams,
          const s32 matching_maxTranslationDistance, const s32 matching_maxProjectiveDistance,
          const s32 verify_maxTranslationDistance, //< How close does a template pixel have to be to an edge to count as a match?
          const u8 verify_maxPixelDifference, //< See verify_numSimilarPixels
          const s32 verify_coordinateIncrement, //< Check every nth row and column (1 is the minimum, 2 is 4x faster, 3 is 9x faster, etc).
          s32 &numMatches,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        Result UpdateTrack_Ransac(
          const Array<u8> &nextImage,
          const EdgeDetectionParameters &edgeDetectionParams,
          const s32 matching_maxProjectiveDistance,
          const s32 verify_maxTranslationDistance, //< How close does a template pixel have to be to an edge to count as a match?
          const u8 verify_maxPixelDifference, //< See verify_numSimilarPixels
          const s32 verify_coordinateIncrement, //< Check every nth row and column (1 is the minimum, 2 is 4x faster, 3 is 9x faster, etc).
          const s32 ransac_maxIterations,
          const s32 ransac_numSamplesPerType, //< for four types
          const s32 ransac_inlinerDistance,
          s32 &numMatches,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        bool IsValid() const;

        Result ShowTemplate(const char * windowName="BinaryTracker Template", const bool waitForKeypress=false, const bool fitImageToWindow=false, const f32 displayScale=1.0f) const;

        // Update the transformation. The format of the update should be as follows:
        // TRANSFORM_TRANSLATION: [-dx, -dy]
        // TRANSFORM_AFFINE: [h00, h01, h02, h10, h11, h12]
        // TRANSFORM_PROJECTIVE: [h00, h01, h02, h10, h11, h12, h20, h21]
        Result UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType=Transformations::TRANSFORM_UNKNOWN);

        Result Serialize(const char *objectName, SerializedBuffer &buffer) const;
        Result Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack &memory);

        s32 get_numTemplatePixels() const;

        Result set_transformation(const Transformations::PlanarTransformation_f32 &transformation);
        Transformations::PlanarTransformation_f32 get_transformation() const;

        s32 get_serializationSize() const;

        // The last used threshold is the last threshold that was used to binarize an image
        // The last threshold is the value computed on the last image, that will be used for the next image
        s32 get_lastUsedGrayvalueThrehold() const;
        s32 get_lastGrayvalueThreshold() const;

      protected:
        typedef struct
        {
          Array<s32> xDecreasing_yStartIndexes;
          Array<s32> xIncreasing_yStartIndexes;
          Array<s32> yDecreasing_xStartIndexes;
          Array<s32> yIncreasing_xStartIndexes;
        } AllIndexLimits;

        typedef struct
        {
          Point<f32> templatePoint;
          Point<f32> matchedPoint;
        } IndexCorrespondence;

        enum UpdateVersion
        {
          UPDATE_VERSION_NORMAL = 1,
          UPDATE_VERSION_LIST = 2,
          UPDATE_VERSION_RANSAC = 3
        };

        Array<u8> templateImage;
        s32 templateImageHeight;
        s32 templateImageWidth;
        Quadrilateral<f32> templateQuad;
        IntegerCounts templateIntegerCounts;

        // The indexes of the detected edges
        EdgeLists templateEdges;

        f32 originalTemplateOrientation;

        Transformations::PlanarTransformation_f32 transformation;

        // The last used threshold is the last threshold that was used to binarize an image
        // The last threshold is the value computed on the last image, that will be used for the next image
        u8 lastUsedGrayvalueThreshold;
        u8 lastGrayvalueThreshold;

        bool isValid;

        // Find the min and max indexes of point with a given Y location
        // The list of points must be sorted in Y, from low to high
        static Result ComputeIndexLimitsVertical(const FixedLengthList<Point<s16> > &points, Array<s32> &yStartIndexes);

        // Find the min and max indexes of point with a given X location
        // The list of points must be sorted in X, from low to high
        static Result ComputeIndexLimitsHorizontal(const FixedLengthList<Point<s16> > &points, Array<s32> &xStartIndexes);

        NO_INLINE static Result FindVerticalCorrespondences_Translation(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
          s32 &sumY,
          s32 &numCorrespondences);

        NO_INLINE static Result FindHorizontalCorrespondences_Translation(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          s32 &sumX,
          s32 &numCorrespondences);

        NO_INLINE static Result FindVerticalCorrespondences_Projective(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
          Array<f32> &AtA,
          Array<f32> &Atb_t);

        NO_INLINE static Result FindHorizontalCorrespondences_Projective(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          Array<f32> &AtA,
          Array<f32> &Atb_t);

        NO_INLINE static Result FindVerticalCorrespondences_Verify(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
          s32 &numTemplatePixelsMatched);

        NO_INLINE static Result FindHorizontalCorrespondences_Verify(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          s32 &numTemplatePixelsMatched);

        // Note: Clears the list matchingIndexes before starting
        NO_INLINE static Result FindVerticalCorrespondences_List(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
          FixedLengthList<IndexCorrespondence> &matchingIndexes);

        // Note: Clears the list matchingIndexes before starting
        NO_INLINE static Result FindHorizontalCorrespondences_List(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          FixedLengthList<IndexCorrespondence> &matchingIndexes);

        NO_INLINE static Result ApplyVerticalCorrespondenceList_Projective(
          const FixedLengthList<IndexCorrespondence> &matchingIndexes,
          Array<f32> &AtA,
          Array<f32> &Atb_t);

        NO_INLINE static Result ApplyHorizontalCorrespondenceList_Projective(
          const FixedLengthList<IndexCorrespondence> &matchingIndexes,
          Array<f32> &AtA,
          Array<f32> &Atb_t);

        // Allocates allLimits, and computes all the indexe limits
        static Result ComputeAllIndexLimits(const EdgeLists &imageEdges, AllIndexLimits &allLimits, MemoryStack &memory);

        Result IterativelyRefineTrack_Translation(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance,
          MemoryStack scratch);

        Result IterativelyRefineTrack_Projective(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance,
          MemoryStack scratch);

        // WARNING: List projective is slower than non-list, but is useful for RANSAC-type algorithms
        Result IterativelyRefineTrack_Projective_List(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance,
          const s32 maxMatchesPerType,
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        Result IterativelyRefineTrack_Projective_Ransac(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance,
          const s32 maxMatchesPerType,
          const s32 ransac_maxIterations,
          const s32 ransac_numSamplesPerType, //< for four types
          const s32 ransac_inlinerDistance,
          s32 &bestNumInliers,
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        // Only checks for the first match for each template pixel, within matching_maxDistance pixels.
        // For each template pixel matched, numTemplatePixelsMatched is incremented
        Result VerifyTrack(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance,
          s32 &numTemplatePixelsMatched);

        Result UpdateTrack_Generic(
          const UpdateVersion version,
          const Array<u8> &nextImage,
          const EdgeDetectionParameters &edgeDetectionParams,
          const s32 matching_maxTranslationDistance, const s32 matching_maxProjectiveDistance,
          const s32 verify_maxTranslationDistance, const u8 verify_maxPixelDifference, const s32 verify_coordinateIncrement,
          const s32 ransac_maxIterations,
          const s32 ransac_numSamplesPerType, //< for four types
          const s32 ransac_inlinerDistance,
          s32 &numMatches,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack fastScratch,
          MemoryStack slowScratch);
      }; // class BinaryTracker
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_BINARY_TRACKER_H_
