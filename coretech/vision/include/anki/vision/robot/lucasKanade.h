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

      // Updates the previous corners with the new transformation
      // and returns the minimum distance any corner moved
      f32 UpdatePreviousCorners(
        const Transformations::PlanarTransformation_f32 &transformation,
        FixedLengthList<Quadrilateral<f32> > &previousCorners,
        MemoryStack scratch);

      class LucasKanadeTracker_Slow
      {
        // The generic LucasKanadeTracker class can track a template with the Lucas-Kanade method,
        // either with translation-only, affine, or projective updates. The two main steps are
        // initialization and update.
        //
        // NOTE:
        // This class uses a lot of memory (on the order of 600kb for an 80x60 input).

      public:
        LucasKanadeTracker_Slow();
        LucasKanadeTracker_Slow(const Array<u8> &templateImage, const Quadrilateral<f32> &templateRegion, const s32 numPyramidLevels, const Transformations::TransformType transformType, const f32 ridgeWeight, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, const bool useWeights, bool& converged, MemoryStack scratch);

        bool IsValid() const;

        // Update the transformation. The format of the update should be as follows:
        // TRANSFORM_TRANSLATION: [-dx, -dy]
        // TRANSFORM_AFFINE: [h00, h01, h02, h10, h11, h12]
        // TRANSFORM_PROJECTIVE: [h00, h01, h02, h10, h11, h12, h20, h21]
        Result UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType=Transformations::TRANSFORM_UNKNOWN);

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
        // NOTE: These are in the downsampled template coordinates
        f32 templateRegionHeight;
        f32 templateRegionWidth;

        Transformations::PlanarTransformation_f32 transformation;

        f32 ridgeWeight;

        f32 templateWeightsSigma;

        // NOTE: templateRegion is in the downsampled template coordinates
        Rectangle<f32> templateRegion;

        bool isValid;
        bool isInitialized;

        // templateImage is the input to initialize the tracking.
        //
        // Allocated some permanant structures using memory, as well as some temporary structures. As a result, it should only be called once.
        Result InitializeTemplate(const Array<u8> &templateImage, MemoryStack &memory);

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, const bool useWeights, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTracker_Slow

      class LucasKanadeTracker_Fast
      {
      public:
        LucasKanadeTracker_Fast(const Transformations::TransformType maxSupportedTransformType);
        LucasKanadeTracker_Fast(const Transformations::TransformType maxSupportedTransformType, const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const Transformations::TransformType transformType, MemoryStack &memory);

        bool IsValid() const;

        // Update the transformation. The format of the update should be as follows:
        // TRANSFORM_TRANSLATION: [-dx, -dy]
        // TRANSFORM_AFFINE: [h00, h01, h02, h10, h11, h12]
        // TRANSFORM_PROJECTIVE: [h00, h01, h02, h10, h11, h12, h20, h21]
        Result UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType=Transformations::TRANSFORM_UNKNOWN);

        Result set_transformation(const Transformations::PlanarTransformation_f32 &transformation);

        Transformations::PlanarTransformation_f32 get_transformation() const;

      protected:
        Transformations::TransformType maxSupportedTransformType;

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

        // Template region coordinates are scaled from the standard resolution
        // by templateImage.get_size(1) / BASE_IMAGE_WIDTH
        Rectangle<f32> templateRegion;

        bool isValid;

        //Result Initialize(const Transformations::TransformType maxSupportedTransformType, const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const Transformations::TransformType transformType, MemoryStack &memory);
      }; // class LucasKanadeTracker_Fast

      class LucasKanadeTracker_Affine : public LucasKanadeTracker_Fast
      {
        // An Translation-only or Affine-plus-translation LucasKanadeTracker. Unlike the general
        // LucasKanadeTracker, this version uses much less memory, and could be better optimized.

      public:
        LucasKanadeTracker_Affine();
        LucasKanadeTracker_Affine(const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const Transformations::TransformType transformType, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, bool& converged, MemoryStack scratch);

      protected:
        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTracker_Affine

      class LucasKanadeTracker_Projective : public LucasKanadeTracker_Fast
      {
        // A Projective-plus-translation LucasKanadeTracker. Unlike the general LucasKanadeTracker,
        // this version uses much less memory, and could be better optimized.

      public:
        LucasKanadeTracker_Projective();
        LucasKanadeTracker_Projective(const Array<u8> &templateImage, const Quadrilateral<f32> &templateQuad, const s32 numPyramidLevels, const Transformations::TransformType transformType, MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, bool& converged, MemoryStack scratch);

      protected:
        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Projective(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTracker_Projective
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_LUCAS_KANADE_H_
