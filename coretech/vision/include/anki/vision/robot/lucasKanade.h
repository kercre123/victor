/**
File: lucasKanade.h
Author: Peter Barnum
Created: 2013

Template tracking with the Lucas-Kanade gradient descent method.

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef _ANKICORETECHEMBEDDED_VISION_LUCAS_KANADE_H_
#define _ANKICORETECHEMBEDDED_VISION_LUCAS_KANADE_H_

#include "anki/common/robot/config.h"
#include "anki/vision/robot/edgeDetection.h"
#include "anki/vision/robot/transformations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      const s32 NUM_PREVIOUS_QUADS_TO_COMPARE = 2;

      class LucasKanadeTracker_f32
      {
        // The generic LucasKanadeTracker class can track a template with the Lucas-Kanade method,
        // either with translation-only, affine, or projective updates. The two main steps are
        // initialization and update.
        //
        // NOTE:
        // This class uses a lot of memory (on the order of 600kb for an 80x60 input).

      public:
        LucasKanadeTracker_f32();
        LucasKanadeTracker_f32(const Array<u8> &templateImage, const Quadrilateral<f32> &templateRegion, const s32 numPyramidLevels, const Transformations::TransformType transformType, const f32 ridgeWeight, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, const bool useWeights, bool& converged, MemoryStack scratch);

        bool IsValid() const;

        Result set_transformation(const Transformations::PlanarTransformation_f32 &transformation);

        Transformations::PlanarTransformation_f32 get_transformation() const;

      protected:
        // A_full is the list of derivative matrices for each level of the pyramid
        FixedLengthList<Array<f32> > A_full;
        FixedLengthList<Array<u8> > templateImagePyramid;
        FixedLengthList<Meshgrid<f32> > templateCoordinates;
        FixedLengthList<Array<f32> > templateWeights;

        s32 numPyramidLevels;

        // The templateImage sizes are the sizes of the image that contains the template
        s32 templateImageHeight;
        s32 templateImageWidth;

        // The templateRegion sizes are the sizes of the part of the template image that will
        // actually be tracked, so must be smaller or equal to the templateImage sizes
        f32 templateRegionHeight;
        f32 templateRegionWidth;

        Transformations::PlanarTransformation_f32 transformation;

        f32 ridgeWeight;

        f32 templateWeightsSigma;

        Rectangle<f32> templateRegion;

        bool isValid;
        bool isInitialized;

        // templateImage is the input to initialize the tracking.
        //
        // Allocated some permanant structures using memory, as well as some temporary structures. As a result, it should only be called once.
        Result InitializeTemplate(const Array<u8> &templateImage, MemoryStack &memory);

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, const bool useWeights, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTracker_f32

      class LucasKanadeTrackerFast
      {
        // An Translation-only or Affine-plus-translation LucasKanadeTracker. Unlike the general
        // LucasKanadeTracker, this version uses much less memory, and could be better optimized.

      public:
        LucasKanadeTrackerFast();
        LucasKanadeTrackerFast(const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const Transformations::TransformType transformType, const f32 ridgeWeight, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, bool& converged, MemoryStack scratch);

        bool IsValid() const;

        Result set_transformation(const Transformations::PlanarTransformation_f32 &transformation);

        Transformations::PlanarTransformation_f32 get_transformation() const;

      protected:
        FixedLengthList<Meshgrid<f32> > templateCoordinates;
        FixedLengthList<Array<u8> > templateImagePyramid;
        FixedLengthList<Array<s16> > templateImageXGradientPyramid;
        FixedLengthList<Array<s16> > templateImageYGradientPyramid;

        s32 numPyramidLevels;

        // The templateImage sizes are the sizes of the image that contains the template
        s32 templateImageHeight;
        s32 templateImageWidth;

        // The templateRegion sizes are the sizes of the part of the template image that will
        // actually be tracked, so must be smaller or equal to the templateImage sizes
        f32 templateRegionHeight;
        f32 templateRegionWidth;

        Transformations::PlanarTransformation_f32 transformation;

        f32 ridgeWeight;

        Rectangle<f32> templateRegion;

        bool isValid;

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTrackerFast

      class LucasKanadeTrackerBinary
      {
        // A binary-image LucasKanade tracker. This is liable to be much faster than the standard
        // LucasKanadeTracker or LucasKanadeTrackerFast, but is also liable to be less accurate and
        // more jittery.
        //
        // Also different, is that the user should call IterativelyRefineTrack as many times as desired. There
        // is no iteration within.

      public:
        LucasKanadeTrackerBinary();

        // the real max number of edge pixels is maxEdgePixelsPerType*4, for each of the four edge types
        LucasKanadeTrackerBinary(
          const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad,
          const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
          MemoryStack &memory);

        // Runs one iteration each of translation and projective
        Result UpdateTrack(
          const Array<u8> &nextImage,
          const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
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

        static Result FindVerticalCorrespondences_Translation(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
          f32 &sumY,
          s32 &numCorrespondences,
          MemoryStack scratch);

        static Result FindHorizontalCorrespondences_Translation(
          const s32 maxMatchingDistance,
          const Transformations::PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
          f32 &sumX,
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
          Array<f32> &Atb,
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
          Array<f32> &Atb,
          MemoryStack scratch);

        // Allocates allLimits, and computes all the indexe limits
        static Result ComputeAllIndexLimits(const EdgeLists &imageEdges, AllIndexLimits &allLimits, MemoryStack &memory);

        Result IterativelyRefineTrack(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
          const Transformations::TransformType updateType,
          MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
          MemoryStack scratch);

        Result IterativelyRefineTrack_Projective(
          const EdgeLists &nextImageEdges,
          const AllIndexLimits &allLimits,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
          MemoryStack scratch);
      }; // class LucasKanadeTrackerBinary
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_LUCAS_KANADE_H_
