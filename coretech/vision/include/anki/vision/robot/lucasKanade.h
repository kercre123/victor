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

      class PlanarTransformation_f32
      {
      public:

        PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, MemoryStack &memory);

        // Initialize with an identity homography
        PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, MemoryStack &memory);

        // Initialize with an identity homography and corners with all zero coordinates
        PlanarTransformation_f32(const TransformType transformType, MemoryStack &memory);

        // Initialize as invalid
        PlanarTransformation_f32();

        // Using the current transformation, warp the In points to the Out points.
        // xIn, yIn, xOut, and yOut must be 1xN.
        // xOut and yOut must be allocated before calling
        // Requires at least N*sizeof(f32) bytes of scratch
        Result TransformPoints(
          const Array<f32> &xIn, const Array<f32> &yIn,
          const f32 scale,
          const Point<f32> &centerOffset,
          Array<f32> &xOut, Array<f32> &yOut) const;

        // Update the transformation. The format of the update should be as follows:
        // TRANSFORM_TRANSLATION: [-dx, -dy]
        Result Update(const Array<f32> &update, MemoryStack scratch, TransformType updateType=TRANSFORM_UNKNOWN);

        Result Print(const char * const variableName = "Transformation");

        Quadrilateral<f32> TransformQuadrilateral(const Quadrilateral<f32> &in, const f32 scale=1.0f) const;

        Result set_transformType(const TransformType transformType);

        TransformType get_transformType() const;

        Result set_homography(const Array<f32>& in);

        const Array<f32>& get_homography() const;

        Result set_initialCorners(const Quadrilateral<f32> &initialCorners);

        const Quadrilateral<f32>& get_initialCorners() const;

        Quadrilateral<f32> get_transformedCorners() const;

      protected:
        TransformType transformType;

        Array<f32> homography; // All types of plane transformations are stored in a 3x3 homography matrix, though some values may be zero (or ones for diagonals)

        Quadrilateral<f32> initialCorners; // The initial corners of the valid region

        static Result TransformPointsStatic(
          const Array<f32> &xIn, const Array<f32> &yIn,
          const f32 scale,
          const Point<f32> &centerOffset,
          Array<f32> &xOut, Array<f32> &yOut,
          const TransformType transformType,
          const Array<f32> &homography);
      };

      class LucasKanadeTracker_f32
      {
      public:
        LucasKanadeTracker_f32(const Array<u8> &templateImage, const Rectangle<f32> &templateRegion, const s32 numPyramidLevels, const TransformType transformType, const f32 ridgeWeight, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, MemoryStack memory);

        bool IsValid() const;

        Result set_transformation(const PlanarTransformation_f32 &transformation);

        PlanarTransformation_f32 get_transformation() const;

      protected:
        // A_full is the list of derivative matrices for each level of the pyramid
        FixedLengthList<Array<f32>> A_full;
        FixedLengthList<Array<u8>> templateImagePyramid;
        FixedLengthList<Meshgrid<f32>> templateCoordinates;
        FixedLengthList<Array<f32>> templateWeights;

        // The templateImage sizes are the sizes of the image that contains the template
        s32 templateImageHeight;
        s32 templateImageWidth;

        s32 numPyramidLevels;

        // The templateRegion sizes are the sizes of the part of the template image that will
        // actually be tracked, so must be smaller or equal to the templateImage sizes
        f32 templateRegionHeight;
        f32 templateRegionWidth;

        PlanarTransformation_f32 transformation;

        f32 ridgeWeight;

        f32 templateWeightsSigma;

        Rectangle<f32> templateRegion;
        Point<f32> center;

        bool isValid;
        bool isInitialized;

        // templateImage is the input to initialize the tracking.
        //
        // Allocated some permanant structures using memory, as well as some temporary structures. As a result, it should only be called once.
        Result InitializeTemplate(const Array<u8> &templateImage, MemoryStack &memory);

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, bool &converged, MemoryStack memory);
      };
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_LUCAS_KANADE_H_
