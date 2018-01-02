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

#include "coretech/common/robot/config.h"
#include "coretech/vision/robot/edgeDetection.h"
#include "coretech/vision/robot/transformations.h"

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      const s32 NUM_PREVIOUS_QUADS_TO_COMPARE = 2;

      // Updates the previous corners with the new transformation
      // and returns the minimum distance any corner moved
      f32 UpdatePreviousCorners(const Transformations::PlanarTransformation_f32 &transformation,
                                const u16 baseImageWidth, const u16 baseImageHeight,
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
        LucasKanadeTracker_Slow(
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateRegion,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const s32 numPyramidLevels,
          const Transformations::TransformType transformType,
          const f32 ridgeWeight,
          MemoryStack &memory);

        Result UpdateTrack(const Array<u8> &nextImage, const s32 maxIterations, const f32 convergenceTolerance, const bool useWeights, bool &verify_converged, MemoryStack scratch);

        bool IsValid() const;

        // Update the transformation. The format of the update should be as follows:
        // TRANSFORM_TRANSLATION: [-dx, -dy]
        // TRANSFORM_AFFINE: [h00, h01, h02, h10, h11, h12]
        // TRANSFORM_PROJECTIVE: [h00, h01, h02, h10, h11, h12, h20, h21]
        Result UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType=Transformations::TRANSFORM_UNKNOWN);

        Result set_transformation(const Transformations::PlanarTransformation_f32 &transformation);

        Transformations::PlanarTransformation_f32 get_transformation() const;

      protected:
        
        u16 baseImageWidth;
        u16 baseImageHeight;
        
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

      // LucasKanadeTracker_Generic handles the transformation-only parts of an LK tracker
      class LucasKanadeTracker_Generic
      {
      public:
        LucasKanadeTracker_Generic(const Transformations::TransformType maxSupportedTransformType);

        LucasKanadeTracker_Generic(
          const Transformations::TransformType maxSupportedTransformType,
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const s32 numPyramidLevels,
          const Transformations::TransformType transformType,
          MemoryStack &memory);

        bool IsValid() const;

        // Update the transformation. The format of the update should be as follows:
        // TRANSFORM_TRANSLATION: [-dx, -dy]
        // TRANSFORM_AFFINE: [h00, h01, h02, h10, h11, h12]
        // TRANSFORM_PROJECTIVE: [h00, h01, h02, h10, h11, h12, h20, h21]
        Result UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType=Transformations::TRANSFORM_UNKNOWN);

        s32 get_numPyramidLevels() const;

        Result set_transformation(const Transformations::PlanarTransformation_f32 &transformation);

        Transformations::PlanarTransformation_f32 get_transformation() const;

      protected:
        // "Base" Image size (i.e. size of the image the initial marker was detected
        // in, which could be different from the tracking resolution)
        // TODO: Probably don't need to support differing detection/tracking resolutions anymore
        u16 baseImageWidth;
        u16 baseImageHeight;
        
        Transformations::TransformType maxSupportedTransformType;

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

        f32 initialImageScaleF32;

        bool isValid;
      }; // class LucasKanadeTracker_Generic

      // LucasKanadeTracker_Fast handles the basic memory of the Fast Affine and Projective trackers (but not the sampled)
      class LucasKanadeTracker_Fast : public LucasKanadeTracker_Generic
      {
      public:
        LucasKanadeTracker_Fast(const Transformations::TransformType maxSupportedTransformType);

        LucasKanadeTracker_Fast(
          const Transformations::TransformType maxSupportedTransformType,
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const s32 numPyramidLevels,
          const Transformations::TransformType transformType,
          MemoryStack &memory);

        bool IsValid() const;

        Result VerifyTrack_Projective(
          const Array<u8> &nextImage,
          const u8 verify_maxPixelDifference,
          s32 &verify_meanAbsoluteDifference,
          s32 &verify_numInBounds,
          s32 &verify_numSimilarPixels,
          MemoryStack scratch) const;

        s32 get_numTemplatePixels() const;

      protected:
        FixedLengthList<Meshgrid<f32> > templateCoordinates;
        FixedLengthList<Array<u8> > templateImagePyramid;
        FixedLengthList<Array<s16> > templateImageXGradientPyramid;
        FixedLengthList<Array<s16> > templateImageYGradientPyramid;
      }; // class LucasKanadeTracker_Fast

      class LucasKanadeTracker_Affine : public LucasKanadeTracker_Fast
      {
        // An Translation-only or Affine-plus-translation LucasKanadeTracker. Unlike the general
        // LucasKanadeTracker, this version uses much less memory, and could be better optimized.

      public:
        LucasKanadeTracker_Affine();

        LucasKanadeTracker_Affine(
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const s32 numPyramidLevels,
          const Transformations::TransformType transformType,
          MemoryStack &memory);

        Result UpdateTrack(
          const Array<u8> &nextImage,
          const s32 maxIterations,
          const f32 convergenceTolerance,
          const u8 verify_maxPixelDifference,
          bool &verify_converged,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack scratch);

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

        LucasKanadeTracker_Projective(
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const s32 numPyramidLevels,
          const Transformations::TransformType transformType,
          MemoryStack &memory);

        Result UpdateTrack(
          const Array<u8> &nextImage,
          const s32 maxIterations,
          const f32 convergenceTolerance,
          const u8 verify_maxPixelDifference,
          bool &verify_converged,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack scratch);

      protected:
        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Projective(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
      }; // class LucasKanadeTracker_Projective

      class LucasKanadeTracker_SampledProjective : public LucasKanadeTracker_Generic
      {
        // A Projective-plus-translation LucasKanadeTracker. Unlike the general LucasKanadeTracker,
        // this version uses much less memory, and could be better optimized.

      public:
        LucasKanadeTracker_SampledProjective();

        LucasKanadeTracker_SampledProjective(
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const s32 numPyramidLevels,
          const Transformations::TransformType transformType,
          const s32 maxSamplesAtBaseLevel,
          MemoryStack ccmMemory,
          MemoryStack &onchipScratch,
          MemoryStack offchipScratch);

        Result UpdateTrack(
          const Array<u8> &nextImage,
          const s32 maxIterations,
          const f32 convergenceTolerance,
          const u8 verify_maxPixelDifference,
          bool &verify_converged,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack scratch);

        bool IsValid() const;

        Result ShowTemplate(const char * windowName="SampledProjective Template", const bool waitForKeypress=false, const bool fitImageToWindow=false) const;

        s32 get_numTemplatePixels(const s32 whichScale) const;

      protected:
        // TODO: verify that there's no alignment padding
        typedef struct TemplateSample
        {
          f32 xCoordinate;
          f32 yCoordinate;
          f32 xGradient;
          f32 yGradient;
          f32 grayvalue;
        } TemplateSample;

        FixedLengthList<FixedLengthList<TemplateSample> > templateSamplePyramid;

        Result VerifyTrack_Projective(
          const Array<u8> &nextImage,
          const u8 verify_maxPixelDifference,
          s32 &verify_meanAbsoluteDifference,
          s32 &verify_numInBounds,
          s32 &verify_numSimilarPixels,
          MemoryStack scratch) const;

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);
        Result IterativelyRefineTrack_Projective(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);

        static Result ApproximateSelect(const Array<f32> &magnitudeVector, const s32 numBins, const s32 numToSelect, s32 &numSelected, Array<u16> &magnitudeIndexes);
      }; // class LucasKanadeTracker_SampledProjective

      class LucasKanadeTracker_SampledPlanar6dof : public LucasKanadeTracker_Generic
      {
        // A Projective-plus-translation LucasKanadeTracker. Unlike the general LucasKanadeTracker,
        // this version uses much less memory, and could be better optimized.

      public:
        LucasKanadeTracker_SampledPlanar6dof();

        LucasKanadeTracker_SampledPlanar6dof(
          const Array<u8> &templateImage,
          const Quadrilateral<f32> &templateQuad,
          const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
          const s32 numPyramidLevels,
          const Transformations::TransformType transformType,
          const s32 numFiducialSquareSamples,
          const Point<f32>& fiducialSquareThicknessFraction,
          const Point<f32>& roundedCornersFraction,
          const s32 maxSamplesAtBaseLevel,
          const s32 numSamplingRegions,
          const f32 focalLength_x,          // TODO: move all this to CTI-Vision-Basestation and use take in CameraCalibration
          const f32 focalLength_y,
          const f32 camCenter_x,
          const f32 camCenter_y,
          const std::vector<f32>& distortionCoeffs,
          const Point<f32>& templateSize_mm, // actual physical size of the template
          MemoryStack ccmMemory,
          MemoryStack &onchipScratch,
          MemoryStack offchipScratch);

        Result UpdateTrack(
          const Array<u8> &nextImage,
          const s32 maxIterations,
          const f32 convergenceTolerance_angle,
          const f32 convergenceTolerance_distance,
          const u8 verify_maxPixelDifference,
          bool &verify_converged,
          s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
          s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
          s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
          MemoryStack scratch);

        // Fill a rotation matrix according to the current tracker angles
        // R should already be allocated to be 3x3
        Result GetRotationMatrix(Array<f32>& R, bool skipLastColumn = false) const;

        // Set the tracker's angles and translation from the given rotation
        // matrix and translation vector. This will in turn update the
        // tracker's transformation (homography).
        Result UpdateRotationAndTranslation(const Array<f32>& R,
          const Point3<f32>& T,
          MemoryStack scratch);

        // Retrieve the current translation estimate of the tracker
        const Point3<f32>& GetTranslation() const;

        // Retrieve the current angle estimates of the tracker
        const f32& get_angleX() const;
        const f32& get_angleY() const;
        const f32& get_angleZ() const;

        // Adjust "proporttional gain" with distance.
        // For example, use this to turn down the proportional gain as we get
        // closer (as indicated by z), to help avoid small oscillations that
        // occur as the target gets large in our field of view.
        void SetGainScheduling(const f32 zMin, const f32 zMax,
          const f32 KpMin, const f32 KpMax);

        bool IsValid() const;

        Result ShowTemplate(const char * windowName="SampledPlanar6dof Template", const bool waitForKeypress=false, const bool fitImageToWindow=false) const;

        s32 get_numTemplatePixels(const s32 whichScale) const;

        // TODO: verify that there's no alignment padding
        typedef struct TemplateSample
        {
          f32 xCoordinate;
          f32 yCoordinate;
          u8  grayvalue;
          f32 A[6];
        } TemplateSample;

        const FixedLengthList<TemplateSample>& get_templateSamples(const s32 atScale) const;

      protected:

        // Store grid of original template values for verification
        // (We don't want to use samples because they are _on_ edges)
        typedef struct VerifySample
        {
          s8 xCoordinate;
          s8 yCoordinate;
          u8 grayvalue;
        } VerifySample;

        f32 verifyCoordScalar;

        // Calibration data:
        f32 focalLength_x;
        f32 focalLength_y;
        f32 camCenter_x;
        f32 camCenter_y;

        // 6DoF Parameters
        typedef struct
        {
          f32 angle_x, angle_y, angle_z;  // rotation
          Point3<f32> translation;
        } Parameters6DoF;

        // Gain scheduling
        f32 Kp_min, Kp_max;
        f32 tz_min, tz_max;
        bool useGainScheduling;
        f32 GetCurrentGain() const;

        Parameters6DoF params6DoF;
        
        //Result SetHomographyFrom6DofParams(Array<f32> &H);

        FixedLengthList<FixedLengthList<TemplateSample> > templateSamplePyramid;
        // FixedLengthList<FixedLengthList<JacobianSample> > jacobianSamplePyramid;
        FixedLengthList<Array<f32> > AtAPyramid;

        // Normalization data
        FixedLengthList<f32> normalizationMean;
        FixedLengthList<f32> normalizationSigmaInv;

        FixedLengthList<VerifySample> verificationSamples;

        // Update the transformation homography with whatever is currently
        // in the 6DoF parameters
        Result UpdateTransformation(MemoryStack scratch);

        // Set the tracker angles by extracting Euler angles from the given
        // rotation matrix
        Result set_rotationAnglesFromMatrix(const Array<f32>& R);

        Result VerifyTrack_Projective(
          const Array<u8> &nextImage,
          const u8 verify_maxPixelDifference,
          s32 &verify_meanAbsoluteDifference,
          s32 &verify_numInBounds,
          s32 &verify_numSimilarPixels,
          MemoryStack scratch);

        Result IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance_angle, const f32 convergenceTolerance_distance, const Transformations::TransformType curTransformType, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &converged, MemoryStack scratch);

        Result IterativelyRefineTrack_Projective(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance_angle, const f32 convergenceTolerance_distance, bool &converged, MemoryStack scratch);

        static Result ApproximateSelect(const Array<f32> &magnitudeVector, const s32 numBins, const s32 numToSelect, s32 &numSelected, Array<s32> &magnitudeIndexes);

        static Result ApproximateSelect(const Array<f32> &magnitudeImage, const s32 numBins, const s32 numRegions, const s32 numToSelect, s32 &numSelected, Array<s32> &magnitudeIndexes);

        Result CreateVerificationSamples(const Array<u8>& image,
          const LinearSequence<f32>& Xlocations,
          const LinearSequence<f32>& Ylocations,
          const f32 verifyCoordScalarInv,
          s32& startIndex,
          MemoryStack scratch);
      }; // class LucasKanadeTracker_SampledPlanar6dof

      // Sparse optical flow, using the KLT tracker
      Result CalcOpticalFlowPyrLK(
        const FixedLengthList<Array<u8> > &prevPyramid, //< Use ImageProcessing::BuildPyramid() to create
        const FixedLengthList<Array<u8> > &nextPyramid, //< Use ImageProcessing::BuildPyramid() to create
        const FixedLengthList<Point<f32> > &prevPoints, //< What are the point locations in the prev image? Use Features::GoodFeaturesToTrack() to detect some good points.
        FixedLengthList<Point<f32> > &nextPoints, //< What are the point locations in the next image? Also see parameter "usePreviousFlowAsInit".
        FixedLengthList<bool> &status, //< Was each point tracked?
        FixedLengthList<f32> &err, //< What is the difference between the template in the previous image and the next image?
        const s32 windowHeight, //< The size of the window around each point
        const s32 windowWidth, //< The size of the window around each point
        const s32 termination_maxCount, //< Max number of iterations for each point
        const f32 termination_epsilon, //< Min movement for a point to count as convergence
        const f32 minEigThreshold, //< If the quality of the pont in the previous image is below this, it is not tracked.
        const bool usePreviousFlowAsInit, //< If usePreviousFlowAsInit == true, these are the initial guesses. If usePreviousFlowAsInit == false, prevPoints is used as the initial guesses.
        MemoryStack scratch); //< Scratch memory
    } // namespace TemplateTracker
  } // namespace Embedded
} //namespace Anki

#endif // _ANKICORETECHEMBEDDED_VISION_LUCAS_KANADE_H_
