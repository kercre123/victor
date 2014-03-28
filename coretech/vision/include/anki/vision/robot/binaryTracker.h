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

#include "anki/common/robot/config.h"
#include "anki/vision/robot/edgeDetection.h"
#include "anki/vision/robot/transformations.h"

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
        BinaryTracker();

        // the real max number of edge pixels is maxEdgePixelsPerType*4, for each of the four edge types
        BinaryTracker(
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          //const u8 edgeDetection_grayvalueThreshold,
          const s32 edgeDetection_threshold_yIncrement, //< How many pixels to use in the y direction (4 is a good value?)
          const s32 edgeDetection_threshold_xIncrement, //< How many pixels to use in the x direction (4 is a good value?)
          const f32 edgeDetection_threshold_blackPercentile, //< What percentile of histogram energy is black? (.1 is a good value)
          const f32 edgeDetection_threshold_whitePercentile, //< What percentile of histogram energy is white? (.9 is a good value)
          const f32 edgeDetection_threshold_scaleRegionPercent, //< How much to scale template bounding box (.8 is a good value)
          const s32 edgeDetection_minComponentWidth, //< The smallest horizontal size of a component (1 to 4 is good)
          const s32 edgeDetection_maxDetectionsPerType, //< As many as you have memory and time for
          const s32 edgeDetection_everyNLines, //< As many as you have time for
          MemoryStack &memory);

        // Runs one iteration each of translation and projective
        //
        // numMatches returns the number of template pixels within verification_maxTranslationDistance of a point in nextImage
        // To check is the update is reasonable, numMatches / this->get_numTemplatePixels() will give the percentage of matches
        Result UpdateTrack(
          const Array<u8> &nextImage,
          const s32 edgeDetection_threshold_yIncrement, //< How many pixels to use in the y direction (4 is a good value?)
          const s32 edgeDetection_threshold_xIncrement, //< How many pixels to use in the x direction (4 is a good value?)
          const f32 edgeDetection_threshold_blackPercentile, //< What percentile of histogram energy is black? (.1 is a good value)
          const f32 edgeDetection_threshold_whitePercentile, //< What percentile of histogram energy is white? (.9 is a good value)
          const f32 edgeDetection_threshold_scaleRegionPercent, //< How much to scale template bounding box (.8 is a good value)
          const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType, const s32 edgeDetection_everyNLines,
          const s32 matching_maxTranslationDistance, const s32 matching_maxProjectiveDistance,
          const s32 verification_maxTranslationDistance,
          const bool useList, //< using a list is liable to be slower
          s32 &numMatches,
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        bool IsValid() const;

        Result ShowTemplate(const char * windowName="BinaryTracker Template", const bool waitForKeypress=false, const bool fitImageToWindow=false) const;

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
          //u16 templateIndex;
          //u16 matchedIndex;
          Point<f32> templatePoint;
          Point<f32> matchedPoint;
        } IndexCorrespondence;

        //Array<u8> templateImage;
        s32 templateImageHeight;
        s32 templateImageWidth;
        //Quadrilateral<f32> templateQuad;

        // The indexes of the detected edges
        EdgeLists templateEdges;

        f32 homographyOffsetX;
        f32 homographyOffsetY;

        Meshgrid<f32> grid;
        Array<f32> xGrid;
        Array<f32> yGrid;

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

        // WARNING: Probably List projective is slower than non-list
        Result IterativelyRefineTrack_List_Projective(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance,
          const s32 maxMatchesPerType,
          MemoryStack fastScratch,
          MemoryStack slowScratch);

        // Only checks for the first match for each template pixel, within matching_maxDistance pixels.
        // For each template pixel matched, numTemplatePixelsMatched is incremented
        Result VerifyTrack(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance,
          s32 &numTemplatePixelsMatched,
          MemoryStack scratch);
      }; // class BinaryTracker
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_BINARY_TRACKER_H_
