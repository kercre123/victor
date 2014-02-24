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
        // LucasKanadeTracker or LucasKanadeTrackerFast, but is also liable to be less accurate and
        // more jittery.

      public:
        BinaryTracker();

        // the real max number of edge pixels is maxEdgePixelsPerType*4, for each of the four edge types
        BinaryTracker(
          const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad,
          const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
          MemoryStack &memory);

        // Runs one iteration each of translation and projective
        Result UpdateTrack(
          const Array<u8> &nextImage,
          const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
          const bool useFixedPoint_projective,
          MemoryStack scratch);

        bool IsValid() const;

        Result ShowTemplate(const bool waitForKeypress, const bool fitImageToWindow) const;

        Result set_transformation(const Transformations::PlanarTransformation_f32 &transformation);
        Transformations::PlanarTransformation_f32 get_transformation() const;

      protected:
        typedef struct
        {
          Array<s32> xDecreasing_yStartIndexes;
          Array<s32> xIncreasing_yStartIndexes;
          Array<s32> yDecreasing_xStartIndexes;
          Array<s32> yIncreasing_xStartIndexes;
        } AllIndexLimits;

        //Array<u8> templateImage;
        s32 templateImageHeight;
        s32 templateImageWidth;
        Quadrilateral<f32> templateQuad;

        // The indexes of the detected edges
        EdgeLists templateEdges;

        f32 homographyOffsetX;
        f32 homographyOffsetY;

        Meshgrid<f32> grid;
        Array<f32> xGrid;
        Array<f32> yGrid;

        Transformations::PlanarTransformation_f32 transformation;

        bool isValid;

#if defined(ANKICORETECH_EMBEDDED_USE_OPENCV) && ANKICORETECH_EMBEDDED_USE_OPENCV
        // Allocates the returned cv::Mat on the heap
        static cv::Mat DrawIndexes(
          const s32 imageHeight, const s32 imageWidth,
          const FixedLengthList<Point<s16> > &indexPoints1,
          const FixedLengthList<Point<s16> > &indexPoints2,
          const FixedLengthList<Point<s16> > &indexPoints3,
          const FixedLengthList<Point<s16> > &indexPoints4);
#endif

        // Find the min and max indexes of point with a given Y location
        // The list of points must be sorted in Y, from low to high
        static Result ComputeIndexLimitsVertical(const FixedLengthList<Point<s16> > &points, Array<s32> &yStartIndexes);

        // Find the min and max indexes of point with a given X location
        // The list of points must be sorted in X, from low to high
        static Result ComputeIndexLimitsHorizontal(const FixedLengthList<Point<s16> > &points, Array<s32> &xStartIndexes);

        /*static Result FindVerticalCorrespondences_Translation(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
        f32 &sumY,
        s32 &numCorrespondences,
        MemoryStack scratch);*/

        static Result FindVerticalCorrespondences_Translation_FixedPoint(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
          s32 &sumY,
          s32 &numCorrespondences,
          MemoryStack scratch);

        /*static Result FindHorizontalCorrespondences_Translation(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
        f32 &sumX,
        s32 &numCorrespondences,
        MemoryStack scratch);*/

        static Result FindHorizontalCorrespondences_Translation_FixedPoint(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          s32 &sumX,
          s32 &numCorrespondences,
          MemoryStack scratch);

        static Result FindVerticalCorrespondences_Projective(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
          Array<f32> &AtA,
          Array<f32> &Atb_t,
          MemoryStack scratch);

        static Result FindVerticalCorrespondences_Projective_FixedPoint(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          Array<f32> &AtA,
          Array<f32> &Atb_t,
          MemoryStack scratch);

        static Result FindHorizontalCorrespondences_Projective(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          Array<f32> &AtA,
          Array<f32> &Atb_t,
          MemoryStack scratch);

        static Result FindHorizontalCorrespondences_Projective_FixedPoint(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          Array<f32> &AtA,
          Array<f32> &Atb_t,
          MemoryStack scratch);

        // Allocates allLimits, and computes all the indexe limits
        static Result ComputeAllIndexLimits(const EdgeLists &imageEdges, AllIndexLimits &allLimits, MemoryStack &memory);

        Result IterativelyRefineTrack_Translation(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
          MemoryStack scratch);

        Result IterativelyRefineTrack_Projective(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
          const bool useFixedPoint,
          MemoryStack scratch);
      }; // class BinaryTracker
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_BINARY_TRACKER_H_
