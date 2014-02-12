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
#include "anki/common/robot/array2d.h"
#include "anki/common/robot/fixedLengthList.h"
#include "anki/common/robot/geometry.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      // The type of transformation.
      //
      // The first byte is the degrees of freedom of the transformation, so if it bit-shifted right
      // by 8, it is equal to the number of parameters
      enum TransformType
      {
        TRANSFORM_UNKNOWN     = 0x0000,
        TRANSFORM_TRANSLATION = 0x0200,
        TRANSFORM_AFFINE      = 0x0600,
        TRANSFORM_PROJECTIVE  = 0x0800
      };

      const s32 NUM_PREVIOUS_QUADS_TO_COMPARE = 2;

      // A PlanarTransformation object can do the following:
      // 1. Hold the current planar transformation, and optionally the initial extents of the quadrilateral
      // 2. Update the planar transformation with an update delta
      // 3. Transform a set of points, quadrilateral, or image to the new coordinate frame
      class PlanarTransformation_f32
      {
      public:

        // Initialize with input corners and homography, or if not input, set to zero or identity
        // If the input corners and homography are input, they are copied to object-local copies
        PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, MemoryStack &memory);
        PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, MemoryStack &memory);
        PlanarTransformation_f32(const TransformType transformType, MemoryStack &memory);

        // Same as the above, but explicitly sets the center of the transformation
        PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, const Point<f32> &centerOffset, MemoryStack &memory);
        PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Point<f32> &centerOffset, MemoryStack &memory);
        PlanarTransformation_f32(const TransformType transformType, const Point<f32> &centerOffset, MemoryStack &memory);

        // Initialize as invalid
        PlanarTransformation_f32();

        // Using the current transformation, warp the In points to the Out points.
        // xIn, yIn, xOut, and yOut must be 1xN.
        // xOut and yOut must be allocated before calling
        // Requires at least N*sizeof(f32) bytes of scratch
        Result TransformPoints(
          const Array<f32> &xIn, const Array<f32> &yIn,
          const f32 scale, Array<f32> &xOut, Array<f32> &yOut) const;

        // Update the transformation. The format of the update should be as follows:
        // TRANSFORM_TRANSLATION: [-dx, -dy]
        // TRANSFORM_AFFINE: [h00, h01, h02, h10, h11, h12]
        // TRANSFORM_PROJECTIVE: [h00, h01, h02, h10, h11, h12, h20, h21]
        Result Update(const Array<f32> &update, MemoryStack scratch, TransformType updateType=TRANSFORM_UNKNOWN);

        Result Print(const char * const variableName = "Transformation");

        // Transform the input Quadrilateral, using this object's transformation
        Quadrilateral<f32> TransformQuadrilateral(const Quadrilateral<f32> &in,
          MemoryStack scratch,
          const f32 scale=1.0f) const;

        // Transform an array (like an image)
        Result TransformArray(const Array<u8> &in,
          Array<u8> &out,
          MemoryStack scratch,
          const f32 scale=1.0f) const;

        bool IsValid() const;

        // Set this object's transformType, centerOffset, initialCorners, and homography
        Result Set(const PlanarTransformation_f32 &newTransformation);

        Result set_transformType(const TransformType transformType);
        TransformType get_transformType() const;

        Result set_homography(const Array<f32>& in);
        const Array<f32>& get_homography() const;

        Result set_initialCorners(const Quadrilateral<f32> &initialCorners);
        const Quadrilateral<f32>& get_initialCorners() const;

        const Point<f32>& get_centerOffset() const;

        // Transform this object's initialCorners, based on its current homography
        Quadrilateral<f32> get_transformedCorners(MemoryStack scratch) const;

      protected:
        bool isValid;

        TransformType transformType;

        // All types of plane transformations are stored in a 3x3 homography matrix, though some values may be zero (or ones for diagonals)
        Array<f32> homography;

        // The initial corners of the valid region
        Quadrilateral<f32> initialCorners;

        // The offset applied to an image, so that origin of the coordinate system is at the center
        // of the quadrilateral
        Point<f32> centerOffset;

        static Result TransformPointsStatic(
          const Array<f32> &xIn, const Array<f32> &yIn,
          const f32 scale,
          const Point<f32>& centerOffset,
          Array<f32> &xOut, Array<f32> &yOut,
          const TransformType transformType,
          const Array<f32> &homography);

        Result Init(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, const Point<f32> &centerOffset, MemoryStack &memory);
      };

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
        LucasKanadeTracker_f32(const Array<u8> &templateImage, const Quadrilateral<f32> &templateRegion, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, const bool useWeights, bool& converged, MemoryStack scratch);

        bool IsValid() const;

        Result set_transformation(const PlanarTransformation_f32 &transformation);

        PlanarTransformation_f32 get_transformation() const;

      protected:
        // A_full is the list of derivative matrices for each level of the pyramid
        FixedLengthList<Array<f32>> A_full;
        FixedLengthList<Array<u8>> templateImagePyramid;
        FixedLengthList<Meshgrid<f32>> templateCoordinates;
        FixedLengthList<Array<f32>> templateWeights;

        s32 numPyramidLevels;

        // The templateImage sizes are the sizes of the image that contains the template
        s32 templateImageHeight;
        s32 templateImageWidth;

        // The templateRegion sizes are the sizes of the part of the template image that will
        // actually be tracked, so must be smaller or equal to the templateImage sizes
        f32 templateRegionHeight;
        f32 templateRegionWidth;

        PlanarTransformation_f32 transformation;

        f32 ridgeWeight;

        f32 templateWeightsSigma;

        Rectangle<f32> templateRegion;

        bool isValid;
        bool isInitialized;

        // templateImage is the input to initialize the tracking.
        //
        // Allocated some permanant structures using memory, as well as some temporary structures. As a result, it should only be called once.
        Result InitializeTemplate(const Array<u8> &templateImage, MemoryStack &memory);

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, const bool useWeights, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTracker_f32

      class LucasKanadeTrackerFast
      {
        // An Translation-only or Affine-plus-translation LucasKanadeTracker. Unlike the general
        // LucasKanadeTracker, this version uses much less memory, and could be better optimized.

      public:
        LucasKanadeTrackerFast();
        LucasKanadeTrackerFast(const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, bool& converged, MemoryStack scratch);

        bool IsValid() const;

        Result set_transformation(const PlanarTransformation_f32 &transformation);

        PlanarTransformation_f32 get_transformation() const;

      protected:
        FixedLengthList<Meshgrid<f32>> templateCoordinates;
        FixedLengthList<Array<u8>> templateImagePyramid;
        FixedLengthList<Array<s16>> templateImageXGradientPyramid;
        FixedLengthList<Array<s16>> templateImageYGradientPyramid;

        s32 numPyramidLevels;

        // The templateImage sizes are the sizes of the image that contains the template
        s32 templateImageHeight;
        s32 templateImageWidth;

        // The templateRegion sizes are the sizes of the part of the template image that will
        // actually be tracked, so must be smaller or equal to the templateImage sizes
        f32 templateRegionHeight;
        f32 templateRegionWidth;

        PlanarTransformation_f32 transformation;

        f32 ridgeWeight;

        Rectangle<f32> templateRegion;

        bool isValid;

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTrackerFast

      class LucasKanadeTrackerBinary
      {
        // A binary-image LucasKanade tracker. This is liable to be much faster than the standard
        // LucasKanadeTracker or LucasKanadeTrackerFast, but is also liable to be less accurate and
        // more jittery.
        //
        // Also different, is that the user should call UpdateTrackOnce as many times as desired. There
        // is no iteration within.

      public:
        class Correspondence
        {
        public:
          Point<f32> originalTemplatePoint;
          Point<f32> warpedTemplatePoint;
          Point<f32> matchedPoint;
          bool isVerticalMatch;
        };

        // Find the min and max indexes of point with a given Y location
        // The list of points must be sorted in Y, from low to high
        static Result ComputeIndexLimitsVertical(const FixedLengthList<Point<s16> > &points, Array<s32> &yStartIndexes);

        // Find the min and max indexes of point with a given X location
        // The list of points must be sorted in X, from low to high
        static Result ComputeIndexLimitsHorizontal(const FixedLengthList<Point<s16> > &points, Array<s32> &xStartIndexes);

        static Result FindVerticalCorrespondences(
          const s32 maxMatchingDistance,
          const PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          FixedLengthList<LucasKanadeTrackerBinary::Correspondence> &correspondences,
          MemoryStack scratch);

        static Result FindHorizontalCorrespondences(
          const s32 maxMatchingDistance,
          const PlanarTransformation_f32 &transformation,
          const FixedLengthList<Point<s16> > &templatePoints,
          const FixedLengthList<Point<s16> > &newPoints,
          const s32 imageHeight,
          const s32 imageWidth,
          FixedLengthList<LucasKanadeTrackerBinary::Correspondence> &correspondences,
          MemoryStack scratch);

        LucasKanadeTrackerBinary();

        // the real max number of edge pixels is maxEdgePixelsPerType*4, for each of the four edge types
        LucasKanadeTrackerBinary(
          const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad,
          const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
          MemoryStack &memory);

        Result UpdateTrackOnce(
          const Array<u8> &nextImage,
          const u8 edgeDetection_grayvalueThreshold, const s32 edgeDetection_minComponentWidth, const s32 edgeDetection_maxDetectionsPerType,
          const s32 matching_maxDistance, const s32 matching_maxCorrespondences,
          const TransformType updateType,
          MemoryStack scratch);

        bool IsValid() const;

        Result ShowTemplate(const bool waitForKeypress, const bool fitImageToWindow) const;

        Result set_transformation(const PlanarTransformation_f32 &transformation);
        PlanarTransformation_f32 get_transformation() const;

      protected:
        Array<u8> templateImage;
        Quadrilateral<f32> templateQuad;

        // The indexes of the detected edges
        FixedLengthList<Point<s16> > template_xDecreasingIndexes;
        FixedLengthList<Point<s16> > template_xIncreasingIndexes;
        FixedLengthList<Point<s16> > template_yDecreasingIndexes;
        FixedLengthList<Point<s16> > template_yIncreasingIndexes;

        f32 homographyOffsetX;
        f32 homographyOffsetY;

        Meshgrid<f32> grid;
        Array<f32> xGrid;
        Array<f32> yGrid;

        PlanarTransformation_f32 transformation;

        bool isValid;

#ifdef ANKICORETECH_EMBEDDED_USE_OPENCV
        // Allocates the returned cv::Mat on the heap
        static cv::Mat LucasKanadeTrackerBinary::DrawIndexes(
          const s32 imageHeight, const s32 imageWidth,
          const FixedLengthList<Point<s16> > &indexPoints1,
          const FixedLengthList<Point<s16> > &indexPoints2,
          const FixedLengthList<Point<s16> > &indexPoints3,
          const FixedLengthList<Point<s16> > &indexPoints4);
#endif

        Result UpdateTransformation(const FixedLengthList<LucasKanadeTrackerBinary::Correspondence> &correspondences, const TransformType updateType, MemoryStack scratch);
      }; // class LucasKanadeTrackerBinary
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_LUCAS_KANADE_H_
