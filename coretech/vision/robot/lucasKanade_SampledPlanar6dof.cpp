/**
File: lucasKanade_SampledPlanar6dof.cpp
Author: Andrew Stein
Created: 2014-04-04

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed
non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/lucasKanade.h"
#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/robot/interpolate.h"
#include "coretech/common/robot/arrayPatterns.h"
#include "coretech/common/robot/find.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/draw.h"
#include "coretech/common/robot/comparisons.h"
#include "coretech/common/robot/errorHandling.h"

#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/imageProcessing.h"
#include "coretech/vision/robot/transformations.h"
#include "coretech/vision/robot/perspectivePoseEstimation.h"

#include <array>

//#define SEND_BINARY_IMAGES_TO_MATLAB

#define USE_OPENCV_ITERATIVE_POSE_INIT 1

#define COMPUTE_CONVERGENCE_FROM_CORNER_CHANGE 0

#define USE_INTENSITY_NORMALIZATION 0

// If 0, just uses nearest pixel
#define USE_LINEAR_INTERPOLATION_FOR_VERIFICATION 0

#define UPDATE_VERIFICATION_SAMPLES_DURING_TRACKING 1

#define SAMPLE_TOP_HALF_ONLY 0

#define USE_BLURRING 0

#if USE_OPENCV_ITERATIVE_POSE_INIT
#include "opencv2/calib3d/calib3d.hpp"
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof()
      : LucasKanadeTracker_Generic(Transformations::TransformType::TRANSFORM_PROJECTIVE)
      {
      }

      // Helper data structures and functions for computing samples
      struct HomographyStruct {
        f32 h00, h01, h02;
        f32 h10, h11, h12;
        f32 h20, h21, h22;

        HomographyStruct(const Array<f32> H, const f32 scale)
        {
          const f32 invScale = 1.f / scale;
          h00 = H[0][0];
          h01 = H[0][1];
          h02 = H[0][2] * invScale;

          h10 = H[1][0];
          h11 = H[1][1];
          h12 = H[1][2] * invScale;

          h20 = H[2][0] * scale;
          h21 = H[2][1] * scale;
          h22 = H[2][2];
        }
      };

      struct dR_dtheta_Struct {
        f32 /*dr11_dthetaX,*/ dr11_dthetaY,   dr11_dthetaZ;
        f32 dr12_dthetaX,     dr12_dthetaY,   dr12_dthetaZ;
        f32 /*dr21_dthetaX,*/ dr21_dthetaY,   dr21_dthetaZ;
        f32 dr22_dthetaX,     dr22_dthetaY,   dr22_dthetaZ;
        f32 /*dr31_dthetaX,*/ dr31_dthetaY  /*dr31_dthetaZ*/;
        f32 dr32_dthetaX,     dr32_dthetaY  /*dr32_dthetaZ*/;

        dR_dtheta_Struct(const f32 angle_x, const f32 angle_y, const f32 angle_z)
        {
          const f32 sx = sinf(angle_x);
          const f32 sy = sinf(angle_y);
          const f32 sz = sinf(angle_z);
          const f32 cx = cosf(angle_x);
          const f32 cy = cosf(angle_y);
          const f32 cz = cosf(angle_z);

          //dr11_dthetaX = 0.f;
          dr12_dthetaX = -sx*sz + cx*sy*cz;
          //dr21_dthetaX = 0.f;
          dr22_dthetaX = -sx*cz - cx*sy*sz;
          //dr31_dthetaX = 0.f;
          dr32_dthetaX = -cx*cy;

          dr11_dthetaY = -sy*cz;
          dr12_dthetaY = sx*cy*cz;
          dr21_dthetaY = sy*sz;
          dr22_dthetaY = -sx*cy*sz;
          dr31_dthetaY = cy;
          dr32_dthetaY = sx*sy;

          dr11_dthetaZ = -cy*sz;
          dr12_dthetaZ = cx*cz - sx*sy*sz;
          dr21_dthetaZ = -cy*cz;
          dr22_dthetaZ = -cx*sz - sx*sy*cz;
          //dr31_dthetaZ = 0.f;
          //dr32_dthetaZ = 0.f;
        }
      };

      inline LucasKanadeTracker_SampledPlanar6dof::TemplateSample ComputeTemplateSample(const u8 grayValue,
        const f32 xOriginal, const f32 yOriginal,
        const f32 xGradient, const f32 yGradient,
        const HomographyStruct& H,
        const dR_dtheta_Struct& dR,
        const f32 focalLength_x, const f32 focalLength_y,
        Array<f32>& AtA)
      {
        LucasKanadeTracker_SampledPlanar6dof::TemplateSample curTemplateSample;

        curTemplateSample.grayvalue   = grayValue;
        curTemplateSample.xCoordinate = xOriginal;
        curTemplateSample.yCoordinate = yOriginal;

        const f32 xTransformedRaw = H.h00*xOriginal + H.h01*yOriginal + H.h02;
        const f32 yTransformedRaw = H.h10*xOriginal + H.h11*yOriginal + H.h12;

        const f32 normalization = H.h20*xOriginal + H.h21*yOriginal + H.h22;

        const f32 invNorm = 1.f / normalization;
        const f32 invNormSq = invNorm *invNorm;

        const f32 dWu_dtx = focalLength_x * invNorm;
        //const f32 dWu_dty = 0.f;
        const f32 dWu_dtz = -xTransformedRaw * invNormSq;

        //const f32 dWv_dtx = 0.f;
        const f32 dWv_dty = focalLength_y * invNorm;
        const f32 dWv_dtz = -yTransformedRaw * invNormSq;

        const f32 r1thetaXterm = /*dr11_dthetaX*xOriginal +*/ dR.dr12_dthetaX*yOriginal;
        const f32 r1thetaYterm = dR.dr11_dthetaY*xOriginal + dR.dr12_dthetaY*yOriginal;
        const f32 r1thetaZterm = dR.dr11_dthetaZ*xOriginal + dR.dr12_dthetaZ*yOriginal;

        const f32 r2thetaXterm = /*dr21_dthetaX*xOriginal + */dR.dr22_dthetaX*yOriginal;
        const f32 r2thetaYterm = dR.dr21_dthetaY*xOriginal + dR.dr22_dthetaY*yOriginal;
        const f32 r2thetaZterm = dR.dr21_dthetaZ*xOriginal + dR.dr22_dthetaZ*yOriginal;

        const f32 r3thetaXterm = /*dr31_dthetaX*xOriginal + */dR.dr32_dthetaX*yOriginal;
        const f32 r3thetaYterm = dR.dr31_dthetaY*xOriginal + dR.dr32_dthetaY*yOriginal;
        //const f32 r3thetaZterm = dr31_dthetaZ*xOriginal + dr32_dthetaZ*yOriginal;

        const f32 dWu_dthetaX = (focalLength_x*normalization*r1thetaXterm -
          r3thetaXterm*xTransformedRaw) * invNormSq;

        const f32 dWu_dthetaY = (focalLength_x*normalization*r1thetaYterm -
          r3thetaYterm*xTransformedRaw) * invNormSq;

        const f32 dWu_dthetaZ = (focalLength_x*normalization*r1thetaZterm
          /* - r3thetaZterm*xTransformedRaw*/) * invNormSq;

        const f32 dWv_dthetaX = (focalLength_y*normalization*r2thetaXterm -
          r3thetaXterm*yTransformedRaw) * invNormSq;

        const f32 dWv_dthetaY = (focalLength_y*normalization*r2thetaYterm -
          r3thetaYterm*yTransformedRaw) * invNormSq;

        const f32 dWv_dthetaZ = (focalLength_y*normalization*r2thetaZterm
          /* - r3thetaZterm*yTransformedRaw*/) * invNormSq;

        // Store the row of the A (Jacobian) matrix for this sample:
        curTemplateSample.A[0] = xGradient*dWu_dthetaX + yGradient*dWv_dthetaX;
        curTemplateSample.A[1] = xGradient*dWu_dthetaY + yGradient*dWv_dthetaY;
        curTemplateSample.A[2] = xGradient*dWu_dthetaZ + yGradient*dWv_dthetaZ;
        curTemplateSample.A[3] = xGradient*dWu_dtx /*+ yGradient*dWv_dtx*/;
        curTemplateSample.A[4] = /*xGradient*dWu_dty + */yGradient*dWv_dty;
        curTemplateSample.A[5] = xGradient*dWu_dtz + yGradient*dWv_dtz;

        // Store the full AtA matrix for when all the samples are within
        // image bounds
        // NOTE: the matrix is symmetric so we only need to compute and
        //       store the 21 upper triangle of unique entries
        for(s32 i=0; i<6; ++i) {
          f32 * restrict AtA_i = AtA.Pointer(i,0);
          for(s32 j=i; j<6; ++j) {
            AtA_i[j]  += curTemplateSample.A[i] * curTemplateSample.A[j];
          }
        }

        return curTemplateSample;
      } // ComputeTemplateSample()

      LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof(
        const Array<u8> &templateImage,
        const Quadrilateral<f32> &templateQuadIn,
        const f32 scaleTemplateRegionPercent,
        const s32 numPyramidLevels,
        const Transformations::TransformType transformType,
        const s32 numFiducialSquareSamples,
        const Point<f32>& fiducialSquareThicknessFraction,
        const Point<f32>& roundedCornersFraction,
        const s32 maxSamplesAtBaseLevel,
        const s32 numSamplingRegions,
        const f32 focalLength_x,
        const f32 focalLength_y,
        const f32 camCenter_x,
        const f32 camCenter_y,
        const std::vector<f32>& distortionCoeffs,
        const Point<f32>& templateSize_mm,
        MemoryStack ccmMemory,
        MemoryStack &onchipScratch,
        MemoryStack offchipScratch)
        : LucasKanadeTracker_Generic(Transformations::TRANSFORM_PROJECTIVE, templateImage, templateQuadIn, scaleTemplateRegionPercent, numPyramidLevels, transformType, onchipScratch)
      {
        // Allocate all the

        //templateImage.Show("InitialImage", false);

        // Initialize calibration data
        this->focalLength_x = focalLength_x;
        this->focalLength_y = focalLength_y;
        this->camCenter_x   = camCenter_x;
        this->camCenter_y   = camCenter_y;

        // Start with gain scheduling off
        this->useGainScheduling = false;

        // Create a canonical 3D template to use.
        Point3<f32> template3d[4];
        const f32 templateHalfWidth  = templateSize_mm.x * 0.5f;
        const f32 templateHalfHeight = templateSize_mm.y * 0.5f;
        template3d[0] = Point3<f32>(-templateHalfWidth, -templateHalfHeight, 0.f);
        template3d[1] = Point3<f32>(-templateHalfWidth,  templateHalfHeight, 0.f);
        template3d[2] = Point3<f32>( templateHalfWidth, -templateHalfHeight, 0.f);
        template3d[3] = Point3<f32>( templateHalfWidth,  templateHalfHeight, 0.f);
        Array<f32> R = Array<f32>(3,3,onchipScratch); // TODO: which scratch?

        // No matter the orientation of the incoming quad, we are always
        // assuming the canonical 3D marker created above is vertically oriented.
        // So, make this one vertically oriented too.
        // TODO: better way to accomplish this?  Feed in un-reordered corners from detected marker?
        Quadrilateral<f32> clockwiseQuad = templateQuadIn.ComputeClockwiseCorners<f32>();
        Quadrilateral<f32> templateQuad(clockwiseQuad[0], clockwiseQuad[3],
          clockwiseQuad[1], clockwiseQuad[2]);

        /*
        const Point<f32> diff03 = templateQuad[0] - templateQuad[3];
        const Point<f32> diff12 = templateQuad[1] - templateQuad[2];
        const f32 templateQuadDiagonal = MAX(diff03.Length(), diff12.Length()) / sqrtf(2.f);
        */

        // Compute the initial pose from the calibration, and the known physical
        // size of the template.  This gives us R matrix and T vector.
        // TODO: is single precision enough for the P3P::computePose call here?
#if USE_OPENCV_ITERATIVE_POSE_INIT
        Result lastResult = RESULT_FAIL;
        {
          cv::Vec3d cvRvec, cvTranslation;

          std::vector<cv::Point2f> cvImagePoints;
          std::vector<cv::Point3f> cvObjPoints;

          for(s32 i=0; i<4; ++i) {
            cvImagePoints.emplace_back(templateQuad[i].get_CvPoint_());
            cvObjPoints.emplace_back(template3d[i].get_CvPoint_());
          }

          Array<f32> calibMatrix = Array<f32>(3,3,onchipScratch);
          calibMatrix[0][0] = focalLength_x; calibMatrix[0][1] = 0.f;           calibMatrix[0][2] = camCenter_x;
          calibMatrix[1][0] = 0.f;           calibMatrix[1][1] = focalLength_y; calibMatrix[1][2] = camCenter_y;
          calibMatrix[2][0] = 0.f;           calibMatrix[2][1] = 0.f;           calibMatrix[2][2] = 1.f;

          cv::Mat_<f32> cvDistortionCoeffs(1, (s32)distortionCoeffs.size(), const_cast<f32*>(distortionCoeffs.data()));
          cv::Mat_<f32> calibMatrix_cvMat(3,3);
          //calibMatrix.ArrayToCvMat(&calibMatrix_cvMat);
          for(s32 i=0; i<3; ++i) {
            for(s32 j=0; j<3; ++j) {
              calibMatrix_cvMat.at<f32>(i,j) = calibMatrix[i][j];
            }
          }

          cv::solvePnP(cvObjPoints, cvImagePoints,
            calibMatrix_cvMat, cvDistortionCoeffs,
            cvRvec, cvTranslation,
            false, CV_ITERATIVE);

          //cv::Matx<float,3,3> cvRmat;
          cv::Mat cvRmat;
          cv::Rodrigues(cvRvec, cvRmat);

          cv::Matx<float,3,3> cvRmatx(cvRmat);

          for(s32 i=0; i<3; ++i) {
            for(s32 j=0; j<3; ++j) {
              R[i][j] = cvRmatx(i,j); //cvRmat.at<float>(i, j);
            }
          }

          this->params6DoF.translation.x = cvTranslation[0];
          this->params6DoF.translation.y = cvTranslation[1];
          this->params6DoF.translation.z = cvTranslation[2];
        }
#else

        Result lastResult = P3P::computePose(templateQuad,
                                             template3d[0], template3d[1],
                                             template3d[2], template3d[3],
                                             this->focalLength_x, this->focalLength_y,
                                             this->camCenter_x,   this->camCenter_y,
                                             R, this->params6DoF.translation);
        
        // NOTE: isValid will still be false in this case.
        AnkiConditionalErrorAndReturn(lastResult == RESULT_OK, "LKTracker_SampledPlanar6dof", "Failed to compute initial pose constructing tracker");

        
#endif // #if USE_OPENCV_ITERATIVE_POSE_INIT

        /*
        R.Print("Initial R for tracker:", 0,3,0,3);
        CoreTechPrint("Initial T for tracker: ");
        this->params6DoF.translation.Print();
        CoreTechPrint("\n");
        */

        // Initialize 6DoF rotation angles
        const Result setR_result = this->set_rotationAnglesFromMatrix(R);
        AnkiConditionalErrorAndReturn(setR_result == RESULT_OK,
          "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
          "Failed to set initial rotation angles.");

        // Initialize the homography from the 6DoF params
        Array<f32> initialHomography = Array<f32>(3,3,onchipScratch); // TODO: on-chip b/c this is permanent, right?

        initialHomography[0][0] = this->focalLength_x*R[0][0];
        initialHomography[0][1] = this->focalLength_x*R[0][1];
        initialHomography[0][2] = this->focalLength_x*this->params6DoF.translation.x;// / initialImageScaleF32;

        initialHomography[1][0] = this->focalLength_y*R[1][0];
        initialHomography[1][1] = this->focalLength_y*R[1][1];
        initialHomography[1][2] = this->focalLength_y*this->params6DoF.translation.y;// / initialImageScaleF32;

        initialHomography[2][0] = R[2][0];// * initialImageScaleF32;
        initialHomography[2][1] = R[2][1];// * initialImageScaleF32;
        initialHomography[2][2] = this->params6DoF.translation.z;

        // Note the center for this tracker is the camera's calibrated center
        Point<f32> centerOffset(this->camCenter_x, this->camCenter_y);

        // Note the initial corners for the transformation are the 3D template
        // corners, since the transformation in this case maps the 3D corners
        // into the image plane
        Quadrilateral<f32> initCorners(Point<float>(template3d[0].x, template3d[0].y),
          Point<float>(template3d[1].x, template3d[1].y),
          Point<float>(template3d[2].x, template3d[2].y),
          Point<float>(template3d[3].x, template3d[3].y));

        this->transformation = Transformations::PlanarTransformation_f32(Transformations::TRANSFORM_PROJECTIVE,
          initCorners, initialHomography, centerOffset,
          onchipScratch); // TODO: which scratch?

        // Important: we have to tell the transformation object that the input
        // points (the canonical 3D model corners) should remain zero-centered
        // and do not need to be recentered every time we apply the current
        // transformation to get the current quad.  This is unlike the other
        // trackers which are generally doing image-to-image transformations
        // instead of 3D-model-to-image transformations.
        this->transformation.set_initialPointsAreZeroCentered(true);

        // Need the partial derivatives at the initial conditions of
        // the rotation parameters:
        dR_dtheta_Struct dR_dtheta(params6DoF.angle_x,
          params6DoF.angle_y,
          params6DoF.angle_z);

        const s32 numSelectBins = 20;

        // TODO: Pass this in as a parameter/argument
        const s32 verifyGridSize = 32;

        BeginBenchmark("LucasKanadeTracker_SampledPlanar6dof");

        this->templateSamplePyramid = FixedLengthList<FixedLengthList<TemplateSample> >(numPyramidLevels, onchipScratch);
        this->verificationSamples   = FixedLengthList<VerifySample>(verifyGridSize*verifyGridSize, onchipScratch);
        this->AtAPyramid            = FixedLengthList<Array<f32> >(numPyramidLevels, onchipScratch);

        AnkiConditionalErrorAndReturn(this->templateSamplePyramid.IsValid(),
          "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
          "Unable to allocate templateSamplePyramid FixedLengthList.");

        AnkiConditionalErrorAndReturn(this->verificationSamples.IsValid(),
          "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
          "Unable to allocate verificationSamples FixedLengthList.");

        AnkiConditionalErrorAndReturn(this->AtAPyramid.IsValid(),
          "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
          "Unable to allocate AtAPyramid FixedLengthList.");

        this->templateSamplePyramid.set_size(numPyramidLevels);
        this->AtAPyramid.set_size(numPyramidLevels);

#if USE_INTENSITY_NORMALIZATION
        this->normalizationMean     = FixedLengthList<f32>(numPyramidLevels, onchipScratch);
        this->normalizationSigmaInv = FixedLengthList<f32>(numPyramidLevels, onchipScratch);
        this->normalizationMean.set_size(numPyramidLevels);
        this->normalizationSigmaInv.set_size(numPyramidLevels);
#endif

        //
        // Compute the samples (and their Jacobians) at each scale
        //
        for(s32 iScale=0; iScale<numPyramidLevels; iScale++)
        {
          const f32 scale = static_cast<f32>(1 << iScale);

          this->AtAPyramid[iScale] = Array<f32>(6,6,onchipScratch);
          Array<f32>& curAtA = this->AtAPyramid[iScale];
          AnkiConditionalErrorAndReturn(curAtA.IsValid(),
            "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
            "Invalid AtA matrix at scale %d", iScale);
          curAtA.SetZero();

          const s32 numFiducialSamplesAtScale = numFiducialSquareSamples >> iScale;
          const s32 numFiducialSamplesPerEdge = CeilS32(static_cast<f32>(numFiducialSamplesAtScale)/8.f);
          const s32 actualNumFiducialSamplesAtScale = numFiducialSamplesPerEdge * 8;
          u8 templateBrightValue = 0, templateDarkValue = 0;

          const HomographyStruct currentH(initialHomography, initialImageScaleF32);

          //
          // Samples from the interior of the fiducial
          //
          {
            // Note that template coordinates for this tracker are actually
            // coordinates on the 3d template, which get mapped into the image
            // by the homography
            const f32 halfWidth = scaleTemplateRegionPercent*templateHalfWidth;
            Meshgrid<f32> templateCoordinates = Meshgrid<f32>(Linspace(-halfWidth, halfWidth,
              static_cast<s32>(floorf(this->templateRegionWidth/scale))),
              Linspace(-halfWidth, halfWidth,
              static_cast<s32>(floorf(this->templateRegionHeight/scale))));

            // Halve the sample at each subsequent level (not a quarter)
            const s32 maxPossibleLocations = templateCoordinates.get_numElements();
            const s32 curMaxSamples = MIN(maxPossibleLocations, maxSamplesAtBaseLevel >> iScale);

            this->templateSamplePyramid[iScale] = FixedLengthList<TemplateSample>(curMaxSamples + actualNumFiducialSamplesAtScale, onchipScratch);
            AnkiConditionalErrorAndReturn(this->templateSamplePyramid[iScale].IsValid(),
              "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
              "Unable to allocate templateSamplePyramid[%d] FixedLengthList.", iScale);

            const s32 numPointsY = templateCoordinates.get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates.get_xGridVector().get_size();

            PUSH_MEMORY_STACK(offchipScratch);
            PUSH_MEMORY_STACK(onchipScratch);

            Array<u8>  grayscaleVector(1, numPointsX*numPointsY, offchipScratch);
            Array<f32> xGradientVector(1, numPointsX*numPointsY, offchipScratch);
            Array<f32> yGradientVector(1, numPointsX*numPointsY, offchipScratch);
            Array<s32> magnitudeIndexes(1, numPointsY*numPointsX, offchipScratch); // NOTE: u16 not enough for full 320x240
            s32 numSelected = 0;
            {
              PUSH_MEMORY_STACK(offchipScratch);

              Array<u8> templateImageAtScale(numPointsY, numPointsX, offchipScratch);
              AnkiConditionalErrorAndReturn(templateImageAtScale.IsValid(),
                                            "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                                            "Out of memory allocating templateImageAtScale. Could not allocate %dx%d points.", numPointsX, numPointsY);

#             if USE_BLURRING
              Array<u8> templateImageBoxFiltered(templateImage.get_size(0), templateImage.get_size(1), offchipScratch);
              assert(templateImageBoxFiltered.IsValid());
              
              /*
              s32 boxSize = scale/2;
              if(boxSize %2 == 0) {
                ++boxSize;
              }
              if(boxSize > 2) {
              ImageProcessing::BoxFilter<u8, s32, u8>(templateImage, boxSize, boxSize, templateImageBoxFiltered, offchipScratch);
              } else {
                templateImageBoxFiltered = templateImage;
              }
               */
              cv::Mat_<u8> cvTemplateImage(templateImage.get_size(0), templateImage.get_size(1),
                                           const_cast<u8*>(static_cast<const u8*>(templateImage.get_buffer())));
              cv::GaussianBlur(cvTemplateImage, cvTemplateImage, cv::Size(0,0), static_cast<double>(scale)/3.f);
              
              templateImageBoxFiltered.Set(cvTemplateImage);
              
              if(0){
                FILE *fp;
                char filename[64];
                snprintf(filename, 64, "templateImageBoxFiltered_%d.dat", iScale);
                fp = fopen(filename, "wb");
                fwrite(templateImageBoxFiltered.get_buffer(), templateImageBoxFiltered.get_numElements(), sizeof(u8), fp);
                fclose(fp);
              }
              
              if((lastResult = Interp2_Projective<u8,u8>(templateImageBoxFiltered, templateCoordinates, transformation.get_homography(), this->transformation.get_centerOffset(initialImageScaleF32), templateImageAtScale, INTERPOLATE_LINEAR)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "Interp2_Projective failed with code 0x%x", lastResult);
                return;
              }
              
              if(0){
                cv::Mat_<u8> temp(templateImageAtScale.get_size(0), templateImageAtScale.get_size(1));
                for(s32 i=0; i<temp.rows; ++i) {
                  memcpy(temp[i], templateImageAtScale.Pointer(i,0), temp.cols*sizeof(u8));
                }

                FILE *fp;
                char filename[64];
                snprintf(filename, 64, "templateImageAtScale_%dx%d.dat", templateImageAtScale.get_size(0), templateImageAtScale.get_size(1));
                fp = fopen(filename, "wb");
                fwrite(temp.data, temp.rows*temp.cols, sizeof(u8), fp);
                fclose(fp);
              }
#             else // NOT USE_BLURRING
              
              if((lastResult = Interp2_Projective<u8,u8>(templateImage, templateCoordinates, transformation.get_homography(), this->transformation.get_centerOffset(initialImageScaleF32), templateImageAtScale, INTERPOLATE_LINEAR)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "Interp2_Projective failed with code 0x%x", lastResult);
                return;
              }
              
#             endif // USE_BLURRING
              
              if(numFiducialSamplesAtScale > 0) {
                ConstArraySliceExpression<u8> temp = templateImageAtScale(0,-1,0,-1);
                templateDarkValue   = Matrix::Min(temp);
                templateBrightValue = Matrix::Max(temp);
                AnkiConditionalErrorAndReturn(templateBrightValue > templateDarkValue,
                  "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                  "Bright value (%d) should be larger than dark value (%d).",
                  templateBrightValue, templateDarkValue);
              }

#if USE_INTENSITY_NORMALIZATION
              // Compute the mean and standard deviation of the template for normalization
              f32 tempVariance;
              if((lastResult = Matrix::MeanAndVar<u8,f32>(templateImageAtScale, this->normalizationMean[iScale], tempVariance)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "MeanAndVar failed with code 0x%x", lastResult);
                return;
              }
              AnkiConditionalError(this->normalizationMean[iScale] < 0.f || this->normalizationMean[iScale] > 255.f,
                "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                "Template mean out of reasonable bounds [0,255] - was %.f", this->normalizationMean[iScale]);
              AnkiConditionalError(tempVariance > 0.f,
                "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                "Template variance was zero or negative.");
              this->normalizationSigmaInv[iScale] = 1.f/sqrtf(static_cast<f32>(tempVariance));
#endif

              // Compute X gradient image
              Array<f32> templateImageXGradient(numPointsY, numPointsX, offchipScratch);
              AnkiConditionalErrorAndReturn(templateImageXGradient.IsValid(),
                "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                "Out of memory allocating templateImageXGradient");
              if((lastResult = ImageProcessing::ComputeXGradient<u8,f32,f32>(templateImageAtScale, templateImageXGradient)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "ComputeXGradient failed with code 0x%x", lastResult);
                return;
              }

              // Compuate Y gradient image
              Array<f32> templateImageYGradient(numPointsY, numPointsX, offchipScratch);
              AnkiConditionalErrorAndReturn(templateImageYGradient.IsValid(),
                "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                "Out of memory allocating templateImageYGradient");
              if((lastResult = ImageProcessing::ComputeYGradient<u8,f32,f32>(templateImageAtScale, templateImageYGradient)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "ComputeYGradient failed with code 0x%x", lastResult);
                return;
              }

              const s32 numDesiredSamples = curMaxSamples;

              if(numDesiredSamples < maxPossibleLocations)
                // Using the computed gradients, find a set of the max values, and store them
              {
                PUSH_MEMORY_STACK(offchipScratch);

                /*
                Array<f32> magnitudeVector(1, numPointsY*numPointsX, offchipScratch);
                AnkiConditionalErrorAndReturn(magnitudeVector.IsValid(),
                "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                "Out of memory allocating magnitudeVector");
                */

                {
                  PUSH_MEMORY_STACK(offchipScratch);

                  Array<f32> magnitudeImage(numPointsY, numPointsX, offchipScratch);
                  AnkiConditionalErrorAndReturn(magnitudeImage.IsValid(),
                    "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                    "Out of memory allocating magnitudeImage. Could not allocate %dx%d points.", numPointsX, numPointsY);
                  {
                    PUSH_MEMORY_STACK(offchipScratch);

                    Array<f32> tmpMagnitude(numPointsY, numPointsX, offchipScratch);
                    AnkiConditionalErrorAndReturn(tmpMagnitude.IsValid(),
                      "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                      "Out of memory allocating tmpMagnitude. Could not allocate %dx%d points.", numPointsX, numPointsY);

                    Matrix::DotMultiply<f32,f32,f32>(templateImageXGradient, templateImageXGradient, tmpMagnitude);
                    Matrix::DotMultiply<f32,f32,f32>(templateImageYGradient, templateImageYGradient, magnitudeImage);
                    Matrix::Add<f32,f32,f32>(tmpMagnitude, magnitudeImage, magnitudeImage);
                  } // pop tmpMagnitude

                  //Matrix::Sqrt<f32,f32,f32>(magnitudeImage, magnitudeImage);

                  {
                    PUSH_MEMORY_STACK(offchipScratch);

                    // Rough non-local maxima suppression on magnitudes, to get more
                    // distributed samples, keeping the result in 2D image form
                    Array<bool> magnitudeImageNLMS(numPointsY, numPointsX, offchipScratch);
                    AnkiConditionalErrorAndReturn(magnitudeImageNLMS.IsValid(),
                      "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                      "Out of memory allocating magnitudeImageNLMS. Could not allocate %dx%d points.", numPointsX, numPointsY);
                    magnitudeImageNLMS.SetZero();
                    s32 nonZeroCount = 0;
#if SAMPLE_TOP_HALF_ONLY
                    for(s32 i=1; i<(numPointsY/2)-1; ++i) {
#else
                    for(s32 i=1; i<numPointsY-1; ++i) {
#endif
                      const f32 * restrict mag_iPrev = magnitudeImage.Pointer(i-1,0); // prev
                      const f32 * restrict mag_i     = magnitudeImage.Pointer(i,0);
                      const f32 * restrict mag_iNext = magnitudeImage.Pointer(i+1,0); // next

                      bool * restrict magNLMS_i = magnitudeImageNLMS.Pointer(i,0);

                      for(s32 j=1; j<numPointsX-1; ++j) {
                        const f32 mag      = mag_i[j];
                        const f32 magLeft  = mag_i[j-1];
                        const f32 magRight = mag_i[j+1];
                        const f32 magUp    = mag_iPrev[j];
                        const f32 magDown  = mag_iNext[j];

                        const f32 magUpLeft    = mag_iPrev[j-1];
                        const f32 magUpRight   = mag_iPrev[j+1];
                        const f32 magDownLeft  = mag_iNext[j-1];
                        const f32 magDownRight = mag_iNext[j+1];

                        if((mag > magLeft && mag > magRight) ||
                          (mag > magUp && mag > magDown) ||
                          (mag > magUpLeft && mag > magDownRight) ||
                          (mag > magUpRight && mag > magDownLeft))
                        {
                          magNLMS_i[j] = true; //mag_i[j];
                          ++nonZeroCount;
                        }
                      } // for(s32 j=1; j<numPointsX-1; ++j)
                    } // for(s32 i=1; i<numPointsY-1; ++i)

                    //magnitudeImage.Show("MagImage", false);

                    //if(nonZeroCount > numDesiredSamples) {
                    // After non-local maxima suppression, we still have enough
                    // non-zero magnitudes to do sampling, so actually loop back
                    // through the magnitude image and zero out the non-local
                    // maxima.
                    for(s32 i=1; i<numPointsY-1; ++i) {
                      f32 * restrict mag_i  = magnitudeImage.Pointer(i,0);
                      bool * restrict magNLMS_i = magnitudeImageNLMS.Pointer(i,0);

                      for(s32 j=1; j<numPointsX-1; ++j) {
                        if(!magNLMS_i[j]) {
                          mag_i[j] = 0.f;
                        }
                      }
                      //}
                    } // if(nonZeroCount > numDesiredSamples)

                    //magnitudeImage.Show("MagImageNLMS", false);
                  } // pop magnitudeImageNLMS

                  LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect(magnitudeImage, numSelectBins, numSamplingRegions, numDesiredSamples, numSelected, magnitudeIndexes);
                } // pop magnitudeImage
              } else {
                // There are fewer pixels than desired samples, so just use them
                // all -- no need to sample.
                s32 * restrict pIndex = magnitudeIndexes.Pointer(0,0);
                for(s32 i=0; i<numPointsX*numPointsY; ++i) {
                  pIndex[i] = i;
                }
                numSelected = numPointsX*numPointsY;
              } // if(numDesiredSamples < maxPossibleLocations)

              // Vectorize template image grayvalues, and x/y gradients
              if((lastResult = Matrix::Vectorize<f32,f32>(false, templateImageXGradient, xGradientVector)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                  "Matrix::Vectorize failed with code 0x%x", lastResult);
              }

              if((lastResult = Matrix::Vectorize<f32,f32>(false, templateImageYGradient, yGradientVector)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                  "Matrix::Vectorize failed with code 0x%x", lastResult);
              }

              if((lastResult = Matrix::Vectorize<u8,u8>(false, templateImageAtScale, grayscaleVector)) != RESULT_OK) {
                AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                  "Matrix::Vectorize failed with code 0x%x", lastResult);
              }
            } // pop templateImageAtScale, templateImageXGradient, and templateImageYGradient

            this->templateSamplePyramid[iScale].set_size(numSelected + actualNumFiducialSamplesAtScale);

            if(numSelected == 0) {
              // Should this be an error?
              AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
                "No samples found within given quad at scale %d.", iScale);
            }
            else {
              const s32 numSamples = numSelected;

              //const f32 t2 = GetTimeF32();
              //CoreTechPrint("%f %f\n", t1-t0, t2-t1);

              //{
              //  Matlab matlab(false);
              //  matlab.PutArray(magnitudeVector, "magnitudeVector");
              //  matlab.PutArray(magnitudeIndexes, "magnitudeIndexes");
              //}

              Array<f32> yCoordinatesVector = templateCoordinates.EvaluateY1(false, offchipScratch);
              Array<f32> xCoordinatesVector = templateCoordinates.EvaluateX1(false, offchipScratch);

              const f32 * restrict pYCoordinates = yCoordinatesVector.Pointer(0,0);
              const f32 * restrict pXCoordinates = xCoordinatesVector.Pointer(0,0);
              const f32 * restrict pYGradientVector = yGradientVector.Pointer(0,0);
              const f32 * restrict pXGradientVector = xGradientVector.Pointer(0,0);
              const u8  * restrict pGrayscaleVector = grayscaleVector.Pointer(0,0);
              const s32 * restrict pMagnitudeIndexes = magnitudeIndexes.Pointer(0,0);

              // Fill in these interior samples starting after the on-fiducial samples, if any
              // (even though those are actually filled in below)
              TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[iScale].Pointer(actualNumFiducialSamplesAtScale);

#if USE_INTENSITY_NORMALIZATION
              const f32 curMean     = this->normalizationMean[iScale];
              const f32 curSigmaInv = this->normalizationSigmaInv[iScale];
              const f32 scaleOverFiveTen = (scale / (2.0f*255.0f)) * curSigmaInv;
#else
              const f32 scaleOverFiveTen = scale / (2.0f*255.0f);
#endif

              for(s32 iSample=0; iSample<numSamples; iSample++){
                const s32 curIndex = pMagnitudeIndexes[iSample];

                pTemplateSamplePyramid[iSample] = ComputeTemplateSample(pGrayscaleVector[curIndex],
                  pXCoordinates[curIndex],
                  pYCoordinates[curIndex],
                  scaleOverFiveTen * pXGradientVector[curIndex],
                  scaleOverFiveTen * pYGradientVector[curIndex],
                  currentH, dR_dtheta,
                  this->focalLength_x, this->focalLength_y, curAtA);
              } // for each interior sample
            } // if/else numSelected==0
          } // samples from interior of fiducial

          //
          // Samples from the implicit fiducial edges
          //
          if(numFiducialSquareSamples > 0)
          {
            const u8 fiducialSampleGrayValue = (templateBrightValue + templateDarkValue)/2;

            const f32 contrast = static_cast<f32>(templateBrightValue - templateDarkValue);
            //const f32 derivMagnitude = (0.5f / 255.f) * contrast * templateQuadDiagonal; // NOTE: scale in numerator and denominator cancel
            const f32 derivMagnitude = (0.5f / 255.f) * contrast * scale;

            const f32 innerTemplateWidth = (1.f - 2.f*fiducialSquareThicknessFraction.x) * templateSize_mm.x;
            const f32 innerTemplateHeight = (1.f - 2.f*fiducialSquareThicknessFraction.y) * templateSize_mm.y;
            const f32 innerTemplateHalfWidth = 0.5f * innerTemplateWidth;
            const f32 innerTemplateHalfHeight = 0.5f * innerTemplateHeight;

            // Get pointers for the chunk of the samples corresponding to each
            // edge, so that we can loop over them simultaneously

#if SAMPLE_TOP_HALF_ONLY
            // If only sampling from top half, increment half as fast to use all
            // the samples on only one half.
            const f32 outerInc_x = 0.5f*(1 - 2*roundedCornersFraction.x)*(templateSize_mm.x) / (numFiducialSamplesPerEdge - 1);
            const f32 innerInc_x = 0.5f*(1 - 2*std::max(fiducialSquareThicknessFraction.x, roundedCornersFraction.x)) * (templateSize_mm.x) / (numFiducialSamplesPerEdge-1);
            
            const f32 outerInc_y = 0.5f*(1 - 2*roundedCornersFraction.y)*(templateSize_mm.y) / (numFiducialSamplesPerEdge - 1);
            const f32 innerInc_y = 0.5f*(1 - 2*std::max(fiducialSquareThicknessFraction.y, roundedCornersFraction.y)) * (templateSize_mm.y) / (numFiducialSamplesPerEdge-1);

            // And concentrate twice as many samples on the top line (by
            // splitting in half and using "top" to point to the left side and
            // and "bottom" to point to the right)
            const f32 signFlip = -1.f;
#else
            const f32 outerInc_x = (1 - 2*roundedCornersFraction.x)*(templateSize_mm.x) / (numFiducialSamplesPerEdge - 1);
            const f32 innerInc_x = (1 - 2*std::max(fiducialSquareThicknessFraction.x, roundedCornersFraction.x)) * (templateSize_mm.x) / (numFiducialSamplesPerEdge-1);
            
            const f32 outerInc_y = (1 - 2*roundedCornersFraction.y)*(templateSize_mm.y) / (numFiducialSamplesPerEdge - 1);
            const f32 innerInc_y = (1 - 2*std::max(fiducialSquareThicknessFraction.y, roundedCornersFraction.y)) * (templateSize_mm.y) / (numFiducialSamplesPerEdge-1);
            
            const f32 signFlip = 1.f;
#endif
            TemplateSample * restrict pOuterTop = this->templateSamplePyramid[iScale].Pointer(0);
            TemplateSample * restrict pOuterBtm = this->templateSamplePyramid[iScale].Pointer(1*numFiducialSamplesPerEdge);
            TemplateSample * restrict pInnerTop = this->templateSamplePyramid[iScale].Pointer(2*numFiducialSamplesPerEdge);
            TemplateSample * restrict pInnerBtm = this->templateSamplePyramid[iScale].Pointer(3*numFiducialSamplesPerEdge);

            TemplateSample * restrict pOuterLeft  = this->templateSamplePyramid[iScale].Pointer(4*numFiducialSamplesPerEdge);
            TemplateSample * restrict pOuterRight = this->templateSamplePyramid[iScale].Pointer(5*numFiducialSamplesPerEdge);
            TemplateSample * restrict pInnerLeft  = this->templateSamplePyramid[iScale].Pointer(6*numFiducialSamplesPerEdge);
            TemplateSample * restrict pInnerRight = this->templateSamplePyramid[iScale].Pointer(7*numFiducialSamplesPerEdge);

            f32 outer_x = -(1 - roundedCornersFraction.x)*templateHalfWidth;
            f32 inner_x = -(1 - std::max(roundedCornersFraction.x, fiducialSquareThicknessFraction.x))*innerTemplateHalfWidth;
            
            f32 outer_y = -(1 - roundedCornersFraction.y)*templateHalfHeight;
            f32 inner_y = -(1 - std::max(roundedCornersFraction.y, fiducialSquareThicknessFraction.y))*innerTemplateHalfHeight;

            if(roundedCornersFraction.x == 0 && roundedCornersFraction.y == 0)
            {
              // Top/Bottom Edges' Left Corners:
              pOuterTop[0] = ComputeTemplateSample(fiducialSampleGrayValue, outer_x, -templateHalfHeight,
                -derivMagnitude, -derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerTop[0] = ComputeTemplateSample(fiducialSampleGrayValue, inner_x, -innerTemplateHalfHeight,
                derivMagnitude, derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pOuterBtm[0] = ComputeTemplateSample(fiducialSampleGrayValue, outer_x*signFlip, templateHalfHeight*signFlip,
                -derivMagnitude, derivMagnitude*signFlip,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerBtm[0] = ComputeTemplateSample(fiducialSampleGrayValue, inner_x*signFlip, innerTemplateHalfHeight*signFlip,
                derivMagnitude, -derivMagnitude*signFlip,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              // Left/Right Edges' Top Corners:
              pOuterLeft[0]  = ComputeTemplateSample(fiducialSampleGrayValue, -templateHalfWidth, outer_x,
                -derivMagnitude, -derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pOuterRight[0] = ComputeTemplateSample(fiducialSampleGrayValue, templateHalfWidth, outer_x,
                derivMagnitude, -derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerLeft[0]  = ComputeTemplateSample(fiducialSampleGrayValue, -innerTemplateHalfWidth, inner_x,
                derivMagnitude, derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerRight[0] = ComputeTemplateSample(fiducialSampleGrayValue, innerTemplateHalfWidth, inner_x,
                -derivMagnitude, derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              // Interior of each edge (non-corners)
              outer_x += outerInc_x;
              inner_x += innerInc_x;
              
              outer_y += outerInc_y;
              inner_y += innerInc_y;
            }
            
            for(s32 iSample=1; iSample<numFiducialSamplesPerEdge-1;
              iSample++, outer_x+=outerInc_x, inner_x += innerInc_x)
            {
              // Top / Bottom Edges
              pOuterTop[iSample] = ComputeTemplateSample(fiducialSampleGrayValue, outer_x, -templateHalfHeight,
                0.f, -derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerTop[iSample] = ComputeTemplateSample(fiducialSampleGrayValue, inner_x, -innerTemplateHalfHeight,
                0.f, derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pOuterBtm[iSample] = ComputeTemplateSample(fiducialSampleGrayValue, outer_x*signFlip, templateHalfHeight*signFlip,
                0.f, derivMagnitude*signFlip,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerBtm[iSample] = ComputeTemplateSample(fiducialSampleGrayValue, inner_x*signFlip, innerTemplateHalfHeight*signFlip,
                0.f, -derivMagnitude*signFlip,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);
            }
            
            for(s32 iSample=1; iSample<numFiducialSamplesPerEdge-1;
                iSample++, outer_y+=outerInc_y, inner_y += innerInc_y)
            {
              // Left / Right Edges
              pOuterLeft[iSample]  = ComputeTemplateSample(fiducialSampleGrayValue, -templateHalfWidth, outer_y,
                -derivMagnitude, 0.f,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pOuterRight[iSample] = ComputeTemplateSample(fiducialSampleGrayValue, templateHalfWidth, outer_y,
                derivMagnitude, 0.f,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerLeft[iSample]  = ComputeTemplateSample(fiducialSampleGrayValue, -innerTemplateHalfWidth, inner_y,
                derivMagnitude, 0.f,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerRight[iSample] = ComputeTemplateSample(fiducialSampleGrayValue, innerTemplateHalfWidth, inner_y,
                -derivMagnitude, 0.f,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);
            } // for each interior point

            if(roundedCornersFraction.x == 0 && roundedCornersFraction.y == 0)
            {
              // Top/Bottom Edges' Right Corners:
              pInnerTop[numFiducialSamplesPerEdge-1] = ComputeTemplateSample(fiducialSampleGrayValue, inner_x, -innerTemplateHalfHeight,
                -derivMagnitude, derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pOuterTop[numFiducialSamplesPerEdge-1] = ComputeTemplateSample(fiducialSampleGrayValue, outer_x, -templateHalfHeight,
                derivMagnitude, -derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerBtm[numFiducialSamplesPerEdge-1] = ComputeTemplateSample(fiducialSampleGrayValue, inner_x*signFlip, innerTemplateHalfHeight*signFlip,
                -derivMagnitude, -derivMagnitude*signFlip,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pOuterBtm[numFiducialSamplesPerEdge-1] = ComputeTemplateSample(fiducialSampleGrayValue, outer_x*signFlip, templateHalfHeight*signFlip,
                derivMagnitude, derivMagnitude*signFlip,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              // Left / Right Edges' Bottom Corners
              pOuterLeft[numFiducialSamplesPerEdge-1]  = ComputeTemplateSample(fiducialSampleGrayValue, -templateHalfWidth, outer_x,
                -derivMagnitude, derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pOuterRight[numFiducialSamplesPerEdge-1] = ComputeTemplateSample(fiducialSampleGrayValue, templateHalfWidth, outer_x,
                derivMagnitude, derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerLeft[numFiducialSamplesPerEdge-1]  = ComputeTemplateSample(fiducialSampleGrayValue, -innerTemplateHalfWidth, inner_x,
                derivMagnitude, -derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);

              pInnerRight[numFiducialSamplesPerEdge-1] = ComputeTemplateSample(fiducialSampleGrayValue, innerTemplateHalfWidth, inner_x,
                -derivMagnitude, -derivMagnitude,
                currentH, dR_dtheta,
                this->focalLength_x, this->focalLength_y, curAtA);
            }
          } // if(numFiducialSquareSamples > 0)

          // All interior and fiducial square samples completed, each of which
          // has contributed to the upper triangle of the current AtA matrix
          // at this scale.  Need to fill in the lower triangle to make it
          // symmetric:
          Matrix::MakeSymmetric(curAtA);
        } // for each scale

        //
        // Create Verification Samples
        //
        {
          this->verificationSamples.set_size(verifyGridSize*verifyGridSize);

          const f32 verifyCoordScalarInv = static_cast<f32>(std::numeric_limits<s8>::max()) / templateHalfWidth;

          // Store the scalar we need at tracking time to take the stored s8
          // coordinates into f32 values:
          this->verifyCoordScalar = 1.f / verifyCoordScalarInv;

          s32 interiorStartIndex;
          s32 interiorGridSize;
          f32 interiorHalfWidth, interiorHalfHeight;

          if(numFiducialSquareSamples > 0) {
            // Sample within fiducial, within neighboring interior gap, and the
            // rest from interior region
            interiorGridSize  = verifyGridSize - 4;

            interiorHalfWidth  = 0.5f*templateSize_mm.x*(1.f - 4.f*fiducialSquareThicknessFraction.x);
            interiorHalfHeight = 0.5f*templateSize_mm.y*(1.f - 4.f*fiducialSquareThicknessFraction.y);

            const Point<f32> fiducialSquareCenter(0.5f * (1.f - fiducialSquareThicknessFraction.x)*templateSize_mm.x,
                                               0.5f * (1.f - fiducialSquareThicknessFraction.y)*templateSize_mm.y);
            const Point<f32> gapCenter(0.5f * (1.f - 3.f*fiducialSquareThicknessFraction.x)*templateSize_mm.x,
                                    0.5f * (1.f - 3.f*fiducialSquareThicknessFraction.y)*templateSize_mm.y);
            const Point<f32> gapSidesStart(0.5f * (1.f - 4.f*fiducialSquareThicknessFraction.x)*templateSize_mm.x,
                                        0.5f * (1.f - 4.f*fiducialSquareThicknessFraction.y)*templateSize_mm.y);

            // Top/Bottom Bars of the Square
            interiorStartIndex = 0;
            lastResult = CreateVerificationSamples(templateImage,
              Linspace(-fiducialSquareCenter.x, fiducialSquareCenter.x, verifyGridSize),
#if SAMPLE_TOP_HALF_ONLY
              Linspace(-fiducialSquareCenter.y, -fiducialSquareCenter.y, 1), // top line only
#else
              Linspace(-fiducialSquareCenter.y, fiducialSquareCenter.y, 2),
#endif
              verifyCoordScalarInv,
              interiorStartIndex,
              onchipScratch);

            AnkiConditionalErrorAndReturn(lastResult == RESULT_OK,
              "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
              "CreateVerificationSamples for Top/Bottom Square Bars failed with code 0x%x", lastResult);

            // Left/Right Bars of the Square
            lastResult = CreateVerificationSamples(templateImage,
              Linspace(-fiducialSquareCenter.x, fiducialSquareCenter.x, 2),
#if SAMPLE_TOP_HALF_ONLY
              Linspace(-gapCenter.y, 0.f, verifyGridSize-2),
#else
              Linspace(-gapCenter.y, gapCenter.y, verifyGridSize-2),
#endif
              verifyCoordScalarInv,
              interiorStartIndex,
              onchipScratch);

            AnkiConditionalErrorAndReturn(lastResult == RESULT_OK,
              "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
              "CreateVerificationSamples for Left/Right Square Bars failed with code 0x%x", lastResult);

            // Top/Bottom Bars of the Gap
            lastResult = CreateVerificationSamples(templateImage,
              Linspace(-gapCenter.x, gapCenter.x, verifyGridSize-2),
#if SAMPLE_TOP_HALF_ONLY
              Linspace(-gapCenter.y, -gapCenter.y, 1), // top line only
#else
              Linspace(-gapCenter.y, gapCenter.y, 2),
#endif
              verifyCoordScalarInv,
              interiorStartIndex,
              onchipScratch);

            AnkiConditionalErrorAndReturn(lastResult == RESULT_OK,
              "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
              "CreateVerificationSamples for Top/Bottom Gap Bars failed with code 0x%x", lastResult);

            // Left/Right Bars of the Gap
            lastResult = CreateVerificationSamples(templateImage,
              Linspace(-gapCenter.x, gapCenter.x, 2),
#if SAMPLE_TOP_HALF_ONLY
              Linspace(-gapSidesStart.y, 0.f, verifyGridSize-1),
#else
              Linspace(-gapSidesStart.y, gapSidesStart.y, verifyGridSize-4),
#endif
              verifyCoordScalarInv,
              interiorStartIndex,
              onchipScratch);

            AnkiConditionalErrorAndReturn(lastResult == RESULT_OK,
              "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
              "CreateVerificationSamples for Left/Right Gap Bars failed with code 0x%x", lastResult);
          } else {
            // Use regular grid of samples
            interiorHalfWidth = scaleTemplateRegionPercent*templateHalfWidth;
            interiorHalfHeight = scaleTemplateRegionPercent*templateHalfHeight;
            interiorGridSize  = verifyGridSize;
            interiorStartIndex = 0;
          }

          // Create the interior regular grid of samples
          lastResult = CreateVerificationSamples(templateImage,
            Linspace(-interiorHalfWidth, interiorHalfWidth, interiorGridSize),
#if SAMPLE_TOP_HALF_ONLY
            Linspace(-interiorHalfHeight, 0.f, interiorGridSize),
#else
            Linspace(-interiorHalfHeight, interiorHalfHeight, interiorGridSize),
#endif
            verifyCoordScalarInv,
            interiorStartIndex,
            onchipScratch);

          AnkiConditionalErrorAndReturn(lastResult == RESULT_OK,
            "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof",
            "CreateVerificationSamples failed with code 0x%x", lastResult);
        } // create grid of verification samples

        this->isValid = true;

        EndBenchmark("LucasKanadeTracker_SampledPlanar6dof");
      }

      Result LucasKanadeTracker_SampledPlanar6dof::CreateVerificationSamples(const Array<u8>& image,
        const LinearSequence<f32>& Xlocations,
        const LinearSequence<f32>& Ylocations,
        const f32 verifyCoordScalarInv,
        s32& startIndex,
        MemoryStack scratch)
      {
        //static Matlab matlab(false);
        //matlab.EvalStringEcho("figure, h_axes = axes; hold on");//, imshow(img), hold on");

        Result lastResult = RESULT_OK;

        AnkiConditionalErrorAndReturnValue(startIndex < this->verificationSamples.get_size(),
          RESULT_FAIL_INVALID_PARAMETER,
          "Result LucasKanadeTracker_SampledPlanar6dof::CreateVerificationSamples",
          "Start index not valid.");

        Meshgrid<f32> verifyCoordinates(Xlocations, Ylocations);

        const s32 numSamplesX = verifyCoordinates.get_xGridVector().get_size();
        const s32 numSamplesY = verifyCoordinates.get_yGridVector().get_size();

        Array<f32> verifyGrayscaleVector(1, numSamplesX*numSamplesY, scratch);
        {
          PUSH_MEMORY_STACK(scratch); // necessary?

          Array<f32> verifyImage(numSamplesY, numSamplesX, scratch);

          lastResult = Interp2_Projective<u8,f32>(image, verifyCoordinates,
            this->transformation.get_homography(),
            this->transformation.get_centerOffset(1.f),
            verifyImage, INTERPOLATE_LINEAR);

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
            "LucasKanadeTracker_SampledPlanar6dof::CreateVerificationSamples",
            "Interp2_Projective for verification image failed with code 0x%x", lastResult);

          // Turn verifyImage into a vector
          lastResult = Matrix::Vectorize<f32,f32>(false, verifyImage, verifyGrayscaleVector);

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
            "LucasKanadeTracker_SampledPlanar6dof::CreateVerificationSamples",
            "Matrix::Vectorize failed with code 0x%x", lastResult);
        } // pop verifyImage

        Array<f32> yVerifyVector = verifyCoordinates.EvaluateY1(false, scratch);
        Array<f32> xVerifyVector = verifyCoordinates.EvaluateX1(false, scratch);

        AnkiConditionalErrorAndReturnValue(xVerifyVector.IsValid(), RESULT_FAIL_MEMORY,
          "LucasKanadeTracker_SampledPlanar6dof::CreateVerificationSamples",
          "Failed to allocate xVerifyVector.");

        AnkiConditionalErrorAndReturnValue(yVerifyVector.IsValid(), RESULT_FAIL_MEMORY,
          "LucasKanadeTracker_SampledPlanar6dof::CreateVerificationSamples",
          "Failed to allocate yVerifyVector.");

        const f32 * restrict pYCoordinates = yVerifyVector.Pointer(0,0);
        const f32 * restrict pXCoordinates = xVerifyVector.Pointer(0,0);
        const f32 * restrict pGrayvalues   = verifyGrayscaleVector.Pointer(0,0);

        VerifySample * restrict pVerifySamples = this->verificationSamples.Pointer(0);

        for(s32 i=0; i<numSamplesX*numSamplesY; ++i)
        {
          VerifySample curVerifySample;
          curVerifySample.xCoordinate = static_cast<s8>(pXCoordinates[i] * verifyCoordScalarInv);
          curVerifySample.yCoordinate = static_cast<s8>(pYCoordinates[i] * verifyCoordScalarInv);
          curVerifySample.grayvalue   = static_cast<u8>(pGrayvalues[i]);

          //matlab.EvalStringEcho("plot(%d, %d, 'ro');", curVerifySample.xCoordinate, curVerifySample.yCoordinate);

          pVerifySamples[startIndex + i] = curVerifySample;
        }

        startIndex += numSamplesX*numSamplesY;

        //matlab.EvalStringEcho("set(h_axes, 'XLim', [-130 130], 'YLim', [-130 130]); grid on");

        return RESULT_OK;
      } // CreateVerificationSamples()

      const FixedLengthList<LucasKanadeTracker_SampledPlanar6dof::TemplateSample>& LucasKanadeTracker_SampledPlanar6dof::get_templateSamples(const s32 atScale) const
      {
        AnkiConditionalError(atScale >=0 && atScale < this->templateSamplePyramid.get_size(),
          "LucasKanadeTracker_SampledPlanar6dof::get_templateSamples()",
          "Requested scale out of range.");

        return this->templateSamplePyramid[atScale];
      }

      // Fill a rotation matrix according to the current tracker angles
      // R should already be allocated to be 3x3
      Result LucasKanadeTracker_SampledPlanar6dof::GetRotationMatrix(Array<f32>& R,
        bool skipLastColumn) const
      {
        AnkiConditionalErrorAndReturnValue(AreEqualSize(3, 3, R), RESULT_FAIL_INVALID_SIZE,
          "LucasKanadeTracker_SampledPlanar6dof::GetRotationMatrix",
          "R should be a 3x3 matrix.");

        const f32 cx = cosf(this->params6DoF.angle_x);
        const f32 cy = cosf(this->params6DoF.angle_y);
        const f32 cz = cosf(this->params6DoF.angle_z);
        const f32 sx = sinf(this->params6DoF.angle_x);
        const f32 sy = sinf(this->params6DoF.angle_y);
        const f32 sz = sinf(this->params6DoF.angle_z);

        R[0][0] = cy*cz;
        R[0][1] = cx*sz+sx*sy*cz;
        R[1][0] = -cy*sz;
        R[1][1] = cx*cz-sx*sy*sz;
        R[2][0] = sy;
        R[2][1] = -sx*cy;

        if(!skipLastColumn) {
          R[0][2] = sx*sz-cx*sy*cz;
          R[1][2] = sx*cz+cx*sy*sz;
          R[2][2] = cx*cy;
        }

        return RESULT_OK;
      } // GetRotationMatrix()

      // Set the tracker angles by extracting Euler angles from the given
      // rotation matrix
      Result LucasKanadeTracker_SampledPlanar6dof::set_rotationAnglesFromMatrix(const Array<f32>& R)
      {
        return Matrix::GetEulerAngles(R,
          this->params6DoF.angle_x,
          this->params6DoF.angle_y,
          this->params6DoF.angle_z);
      } // set_rotationAnglesFromMatrix

      // Retrieve/update the current translation estimates of the tracker
      const Point3<f32>& LucasKanadeTracker_SampledPlanar6dof::GetTranslation() const
      {
        return this->params6DoF.translation;
      } // GetTranslation()

      const f32& LucasKanadeTracker_SampledPlanar6dof::get_angleX() const
      {
        return this->params6DoF.angle_x;
      }

      const f32& LucasKanadeTracker_SampledPlanar6dof::get_angleY() const
      {
        return this->params6DoF.angle_y;
      }

      const f32& LucasKanadeTracker_SampledPlanar6dof::get_angleZ() const
      {
        return this->params6DoF.angle_z;
      }

      Result LucasKanadeTracker_SampledPlanar6dof::UpdateTransformation(MemoryStack scratch)
      {
        // Compute the new homography from the new 6DoF parameters, and
        // update the transformation object
        Array<f32> newHomography = Array<f32>(3,3,scratch);
        this->GetRotationMatrix(newHomography, true);

        newHomography[0][0] *= this->focalLength_x;
        newHomography[0][1] *= this->focalLength_x;
        newHomography[0][2] = this->focalLength_x*this->params6DoF.translation.x;// / initialImageScaleF32;

        newHomography[1][0] *= this->focalLength_y;
        newHomography[1][1] *= this->focalLength_y;
        newHomography[1][2] = this->focalLength_y*this->params6DoF.translation.y;// / initialImageScaleF32;

        //newHomography[2][0] = newR[2][0];// * initialImageScaleF32;
        //newHomography[2][1] = newR[2][1];// * initialImageScaleF32;
        newHomography[2][2] = this->params6DoF.translation.z;

        Result result = this->transformation.set_homography(newHomography);

        return result;
      }

      Result LucasKanadeTracker_SampledPlanar6dof::UpdateRotationAndTranslation(const Array<f32>& R,
        const Point3<f32>& T,
        MemoryStack scratch)
      {
        Result lastResult = this->set_rotationAnglesFromMatrix(R);

        if(lastResult == RESULT_OK) {
          this->params6DoF.translation = T;
          lastResult = this->UpdateTransformation(scratch);
        }

        return lastResult;
      }

      Result LucasKanadeTracker_SampledPlanar6dof::ShowTemplate(const char * windowName, const bool waitForKeypress, const bool fitImageToWindow) const
      {
#if !ANKICORETECH_EMBEDDED_USE_OPENCV
        return RESULT_FAIL;
#else
        //if(!this->IsValid())
        //  return RESULT_FAIL;

        const s32 scratchSize = 10000000;
        MemoryStack scratch(malloc(scratchSize), scratchSize);

        Array<u8> image(this->templateImageHeight, this->templateImageWidth, scratch);

        const Point<f32> centerOffset = this->transformation.get_centerOffset(1.0f);

        for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
          const s32 numSamples = this->templateSamplePyramid[iScale].get_size();

          image.SetZero();

          const TemplateSample * restrict pTemplateSample = this->templateSamplePyramid[iScale].Pointer(0);

          for(s32 iSample=0; iSample<numSamples; iSample++) {
            const TemplateSample curTemplateSample = pTemplateSample[iSample];

            const s32 curY = Round<s32>(curTemplateSample.yCoordinate + centerOffset.y);
            const s32 curX = Round<s32>(curTemplateSample.xCoordinate + centerOffset.x);

            if(curX >= 0 && curY >= 0 && curX < this->templateImageWidth && curY < this->templateImageHeight) {
              image[curY][curX] = 255;
            }
          }

          char windowNameTotal[128];
          snprintf(windowNameTotal, 128, "%s %d", windowName, iScale);

          if(fitImageToWindow) {
            cv::namedWindow(windowNameTotal, CV_WINDOW_NORMAL);
          } else {
            cv::namedWindow(windowNameTotal, CV_WINDOW_AUTOSIZE);
          }

          cv::Mat_<u8> image_cvMat;
          ArrayToCvMat(image, &image_cvMat);
          cv::imshow(windowNameTotal, image_cvMat);
        }

        if(waitForKeypress)
          cv::waitKey();

        return RESULT_OK;
#endif // #if !ANKICORETECH_EMBEDDED_USE_OPENCV
      }

      bool LucasKanadeTracker_SampledPlanar6dof::IsValid() const
      {
        if(!LucasKanadeTracker_Generic::IsValid())
          return false;

        // TODO: add more checks

        return true;
      }

      s32 LucasKanadeTracker_SampledPlanar6dof::get_numTemplatePixels(const s32 whichScale) const
      {
        if(whichScale < 0 || whichScale > this->numPyramidLevels)
          return 0;

        return this->templateSamplePyramid[whichScale].get_size();
      }

      Result LucasKanadeTracker_SampledPlanar6dof::UpdateTrack(
        const Array<u8> &nextImage,
        const s32 maxIterations,
        const f32 convergenceTolerance_angle,
        const f32 convergenceTolerance_distance,
        const u8 verify_maxPixelDifference,
        bool &verify_converged,
        s32 &verify_meanAbsoluteDifference,
        s32 &verify_numInBounds,
        s32 &verify_numSimilarPixels,
        MemoryStack scratch)
      {
        Result lastResult;

        Transformations::PlanarTransformation_f32 previousTransformation(this->transformation.get_transformType(), scratch);

#       if USE_BLURRING
        Array<u8> nextImageBlurred(nextImage.get_size(0), nextImage.get_size(1), scratch);
        assert(nextImage.IsValid());
        cv::Mat_<u8> cvNextImage(nextImage.get_size(0), nextImage.get_size(1),
                                 static_cast<u8*>(nextImageBlurred.get_buffer()));
#       endif // USE_BLURRING
        
        for(s32 iScale=numPyramidLevels-1; iScale>=0; iScale--)
        {
#         if USE_BLURRING
          nextImageBlurred.Set(nextImage);
          cv::GaussianBlur(cvNextImage, cvNextImage, cv::Size(0,0), static_cast<double>(1<<iScale)/3.f);
#         define __NEXT_IMAGE__ nextImageBlurred
#         else
#         define __NEXT_IMAGE__ nextImage
#         endif
          
          verify_converged = false;

          previousTransformation.Set(this->transformation);

          BeginBenchmark("UpdateTrack.refineTranslation");
          if((lastResult = IterativelyRefineTrack(__NEXT_IMAGE__, maxIterations, iScale, convergenceTolerance_angle, convergenceTolerance_distance, Transformations::TRANSFORM_TRANSLATION, verify_converged, scratch)) != RESULT_OK)
            return lastResult;
          EndBenchmark("UpdateTrack.refineTranslation");

          if(verify_converged) {
            previousTransformation.Set(this->transformation);

            if(this->transformation.get_transformType() != Transformations::TRANSFORM_TRANSLATION) {
              BeginBenchmark("UpdateTrack.refineOther");
              if((lastResult = IterativelyRefineTrack(__NEXT_IMAGE__, maxIterations, iScale, convergenceTolerance_angle, convergenceTolerance_distance, this->transformation.get_transformType(), verify_converged, scratch)) != RESULT_OK)
                return lastResult;
              EndBenchmark("UpdateTrack.refineOther");

              if(!verify_converged) {
                // If full refinement didn't converge stick with translation
                // only result
                this->transformation.Set(previousTransformation);
              }
            }
          } else {
            // If translation update didn't converge, replace with previous
            // transformation
            this->transformation.Set(previousTransformation);
          }
        } // for(s32 iScale=numPyramidLevels; iScale>=0; iScale--)

        //DEBUG!!!
        //verify_converged = true;

        lastResult = this->VerifyTrack_Projective(__NEXT_IMAGE__, verify_maxPixelDifference, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch);
        
#       undef __NEXT_IMAGE__
        
        return lastResult;
      }

      Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance_angle, const f32 convergenceTolerance_distance, const Transformations::TransformType curTransformType, bool &verify_converged, MemoryStack scratch)
      {
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        AnkiConditionalErrorAndReturnValue(this->IsValid() == true,
          RESULT_FAIL, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "This object is not initialized");

        AnkiConditionalErrorAndReturnValue(nextImage.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "nextImage is not valid");

        AnkiConditionalErrorAndReturnValue(maxIterations > 0 && maxIterations < 1000,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "maxIterations must be greater than zero and less than 1000");

        AnkiConditionalErrorAndReturnValue(whichScale >= 0 && whichScale < this->numPyramidLevels,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "whichScale is invalid");

        AnkiConditionalErrorAndReturnValue(convergenceTolerance_angle > 0.0f && convergenceTolerance_distance > 0.0f,
          RESULT_FAIL_INVALID_PARAMETER, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "convergenceTolerances must be greater than zero");

        AnkiConditionalErrorAndReturnValue(nextImageHeight == templateImageHeight && nextImageWidth == templateImageWidth,
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "nextImage must be the same size as the template");

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const s32 initialImagePowerS32 = Log2u32(static_cast<u32>(initialImageScaleS32));

        AnkiConditionalErrorAndReturnValue(((1<<initialImagePowerS32)*nextImageWidth) == baseImageWidth,
          RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "The templateImage must be a power of two smaller than baseImageWidth (%d)", baseImageWidth);

        if(curTransformType == Transformations::TRANSFORM_TRANSLATION) {
          return IterativelyRefineTrack_Translation(nextImage, maxIterations, whichScale, convergenceTolerance_distance, verify_converged, scratch);
        } else if(curTransformType == Transformations::TRANSFORM_PROJECTIVE) {
          return IterativelyRefineTrack_Projective(nextImage, maxIterations, whichScale, convergenceTolerance_angle, convergenceTolerance_distance, verify_converged, scratch);
        }

        return RESULT_FAIL;
      } // Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const TransformType curTransformType, bool &verify_converged, MemoryStack scratch)

      void LucasKanadeTracker_SampledPlanar6dof::SetGainScheduling(const f32 zMin, const f32 zMax,
        const f32 KpMin, const f32 KpMax)
      {
        this->Kp_min = KpMin;
        this->Kp_max = KpMax;
        this->tz_min = zMin;
        this->tz_max = zMax;

        this->useGainScheduling = true;
      }

      f32 LucasKanadeTracker_SampledPlanar6dof::GetCurrentGain() const
      {
        if(this->useGainScheduling) {
          f32 Kp;
          if(this->params6DoF.translation.z <= this->tz_min) {
            Kp = this->Kp_min;
          }
          else if(this->params6DoF.translation.z >= this->tz_max) {
            Kp = this->Kp_max;
          }
          else {
            Kp = (this->params6DoF.translation.z - this->tz_min)/(this->tz_max-this->tz_min)*(this->Kp_max-this->Kp_min) + this->Kp_min;
          }

          AnkiAssert(Kp >= this->Kp_min && Kp <= this->Kp_max);
          return Kp;
        } else {
          // TODO: make this a parameter
          return 0.25f;
        }
      }

      Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance_distance, bool &verify_converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        Result lastResult;

        Array<f32> AWAt(3, 3, scratch);
        Array<f32> b(1, 3, scratch);

        // Raw symmetric entries
        // Addresses known at compile time, so should be faster
        f32 AWAt00, AWAt01, AWAt02, AWAt11, AWAt12, AWAt22;
        f32 b0, b1, b2;

        verify_converged = false;

        //f32 minChange = 0.f;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const f32 scale = static_cast<f32>(1 << whichScale);

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        //const f32 scaleOverFiveTen = scale / (2.0f*255.0f);

        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);

#if COMPUTE_CONVERGENCE_FROM_CORNER_CHANGE
        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }
#endif

        const f32 xyReferenceMin = 0.f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth - 1);
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight - 1);

        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);

        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);

#if USE_INTENSITY_NORMALIZATION
        const f32 curMean     = this->normalizationMean[whichScale];
        const f32 curSigmaInv = this->normalizationSigmaInv[whichScale];
#endif

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32;
          const f32 h22 = homography[2][2];

          //AWAt.SetZero();
          //b.SetZero();

          // Start with the full (translation-only part of) AtA for this scale.
          // We will subtract out contributions from samples currently out of bounds.
          const Array<f32>& curAtA = this->AtAPyramid[whichScale];
          AWAt00 = curAtA[0][0];
          AWAt01 = curAtA[0][1];
          AWAt02 = curAtA[0][2];
          AWAt11 = curAtA[1][1];
          AWAt12 = curAtA[1][2];
          AWAt22 = curAtA[2][2];

          b0 = 0.f;
          b1 = 0.f;
          b2 = 0.f;

          s32 numInBounds = 0;

          // TODO: make the x and y limits from 1 to end-2

          for(s32 iSample=0; iSample<numTemplateSamples; iSample++) {
            const TemplateSample curSample = pTemplateSamplePyramid[iSample];
            const f32 yOriginal = curSample.yCoordinate;
            const f32 xOriginal = curSample.xCoordinate;

            // TODO: These two could be strength reduced
            const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
            const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

            const f32 normalization = h20*xOriginal + h21*yOriginal + h22;

            const f32 xTransformed = (xTransformedRaw / normalization) + centerOffsetScaled.x;
            const f32 yTransformed = (yTransformedRaw / normalization) + centerOffsetScaled.y;

            const f32 x0 = floorf(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

            const f32 y0 = floorf(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

            // NOTE: The three columns of A we need for translation-only
            // update are the *last* three entries in the sample's A entries
            const f32 A0 = curSample.A[3];
            const f32 A1 = curSample.A[4];
            const f32 A2 = curSample.A[5];

            // If out of bounds, subtract this sample's contribution from AtA
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              //AWAt
              //  b
              AWAt00 -= A0*A0;
              AWAt01 -= A0*A1;
              AWAt02 -= A0*A2;
              AWAt11 -= A1*A1;
              AWAt12 -= A1*A2;
              AWAt22 -= A2*A2;
              continue;
            }

            numInBounds++;

            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1 - alphaX;

            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;

            const s32 y0S32 = Round<s32>(y0);
            const s32 y1S32 = Round<s32>(y1);
            const s32 x0S32 = Round<s32>(x0);

            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);

            f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR,
              alphaY, alphaYinverse, alphaX, alphaXinverse);

#if USE_INTENSITY_NORMALIZATION
            interpolatedPixelF32 = (interpolatedPixelF32 - curMean) * curSigmaInv;
#endif

            //const u8 interpolatedPixel = static_cast<u8>(Round(interpolatedPixelF32));

            // This block is the non-interpolation part of the per-sample algorithm
            {
              const f32 templatePixelValue = static_cast<f32>(curSample.grayvalue);
              //const f32 templatePixelValue = curSample.grayvalue;

              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);

              b0 += A0 * tGradientValue;
              b1 += A1 * tGradientValue;
              b2 += A2 * tGradientValue;
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)
          /*
          if(numInBounds < 16) {
          AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation", "Template drifted too far out of image.");
          return RESULT_OK;
          }
          */
          // Copy out the AtA results for this iteration and create the full
          // symmetric matrix
          AWAt[0][0] = AWAt00;
          AWAt[0][1] = AWAt01;
          AWAt[0][2] = AWAt02;
          AWAt[1][1] = AWAt11;
          AWAt[1][2] = AWAt12;
          AWAt[2][2] = AWAt22;

          Matrix::MakeSymmetric(AWAt, false);

          b[0][0] = b0;
          b[0][1] = b1;
          b[0][2] = b2;

          //AWAt.Print("New AWAt");
          //b.Print("New b");

          bool numericalFailure;

          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false, numericalFailure)) != RESULT_OK)
            return lastResult;

          if(numericalFailure){
            AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation", "numericalFailure");
            return RESULT_OK;
          }

          const f32 Kp = this->GetCurrentGain();

          this->params6DoF.translation.x -= Kp * b[0][0];
          this->params6DoF.translation.y -= Kp * b[0][1];
          this->params6DoF.translation.z -= Kp * b[0][2];

          // Compute the new homography from the new 6DoF parameters
          this->UpdateTransformation(scratch);

          //this->transformation.get_homography().Print("new transformation");

          // Check if we're done with iterations
#if COMPUTE_CONVERGENCE_FROM_CORNER_CHANGE
          f32 minChange = UpdatePreviousCorners(transformation, previousCorners, scratch);

          if(minChange < convergenceTolerance * scale) {
#else
          const f32 transConvergenceTolerance = scale*convergenceTolerance_distance;

          if(fabs(b[0][0]) < transConvergenceTolerance &&
            fabs(b[0][1]) < transConvergenceTolerance &&
            fabs(b[0][2]) < transConvergenceTolerance)
          {
#endif
            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        //CoreTechPrint("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation() "
        //       "failed to converge at scale %f with minChange = %f\n",
        //       scale, minChange);

        return RESULT_OK;
      } // Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation()

      Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Projective(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance_angle, const f32 convergenceTolerance_distance, bool &verify_converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        Result lastResult;

        Array<f32> AWAt(6, 6, scratch);
        Array<f32> b(1, 6, scratch);

        // These addresses should be known at compile time, so should be faster
        //f32 AWAt_raw[6][6];
        const s32 NUM_SYMMETRIC_ENTRIES = 21;
        f32 AWAt_raw[NUM_SYMMETRIC_ENTRIES];
        f32 b_raw[6];

        verify_converged = false;

        // f32 minChange = 0.f;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth  = nextImage.get_size(1);

        const f32 scale = static_cast<f32>(1 << whichScale);

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        //const f32 scaleOverFiveTen = scale / (2.0f*255.0f);

        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);

#if COMPUTE_CONVERGENCE_FROM_CORNER_CHANGE
        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);

        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }
#endif

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth - 1);
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight - 1);

        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);

        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);

#if USE_INTENSITY_NORMALIZATION
        const f32 curMean     = this->normalizationMean[whichScale];
        const f32 curSigmaInv = this->normalizationSigmaInv[whichScale];
#endif

        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32;
          const f32 h22 = homography[2][2];

          //AWAt.SetZero();
          //b.SetZero();

          // Start with the full AtA matrix for this scale. We will subtract out
          // contributions from samples currently out of bounds.
          s32 symIndex = 0;
          for(s32 i=0; i<6; ++i) {
            f32 * restrict AWAt_i = this->AtAPyramid[whichScale].Pointer(i,0);
            for(s32 j=i; j<6; ++j) {
              AWAt_raw[symIndex++] = AWAt_i[j];
            }

            b_raw[i] = 0;
          }

          s32 numInBounds = 0;

          // TODO: make the x and y limits from 1 to end-2

          // DEBUG!!!
          //Array<f32> xTransformedArray = Array<f32>(1,numTemplateSamples,scratch);
          //Array<f32> yTransformedArray = Array<f32>(1,numTemplateSamples,scratch);
          //Array<f32> debugStuff = Array<f32>(numTemplateSamples, 6, scratch);
          //Array<f32> debugA = Array<f32>(numTemplateSamples,6,scratch);
          //static Matlab matlab(false);
          //matlab.PutArray(nextImage, "img");

          for(s32 iSample=0; iSample<numTemplateSamples; iSample++) {
            const TemplateSample curSample = pTemplateSamplePyramid[iSample];
            const f32 yOriginal = curSample.yCoordinate;
            const f32 xOriginal = curSample.xCoordinate;

            // TODO: These two could be strength reduced
            const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
            const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

            const f32 normalization = 1.f / (h20*xOriginal + h21*yOriginal + h22);

            const f32 xTransformed = (xTransformedRaw * normalization) + centerOffsetScaled.x;
            const f32 yTransformed = (yTransformedRaw * normalization) + centerOffsetScaled.y;

            // DEBUG!
            //xTransformedArray[0][iSample] = xTransformed;
            //yTransformedArray[0][iSample] = yTransformed;

            const f32 x0 = floorf(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

            const f32 y0 = floorf(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

            const f32* Arow = curSample.A;

            // If out of bounds, remove this sample's contribution from the
            // AtA matrix
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              AWAt_raw[0]  -= Arow[0] * Arow[0];
              AWAt_raw[1]  -= Arow[0] * Arow[1];
              AWAt_raw[2]  -= Arow[0] * Arow[2];
              AWAt_raw[3]  -= Arow[0] * Arow[3];
              AWAt_raw[4]  -= Arow[0] * Arow[4];
              AWAt_raw[5]  -= Arow[0] * Arow[5];

              AWAt_raw[6]  -= Arow[1] * Arow[1];
              AWAt_raw[7]  -= Arow[1] * Arow[2];
              AWAt_raw[8]  -= Arow[1] * Arow[3];
              AWAt_raw[9]  -= Arow[1] * Arow[4];
              AWAt_raw[10] -= Arow[1] * Arow[5];

              AWAt_raw[11] -= Arow[2] * Arow[2];
              AWAt_raw[12] -= Arow[2] * Arow[3];
              AWAt_raw[13] -= Arow[2] * Arow[4];
              AWAt_raw[14] -= Arow[2] * Arow[5];

              AWAt_raw[15] -= Arow[3] * Arow[3];
              AWAt_raw[16] -= Arow[3] * Arow[4];
              AWAt_raw[17] -= Arow[3] * Arow[5];

              AWAt_raw[18] -= Arow[4] * Arow[4];
              AWAt_raw[19] -= Arow[4] * Arow[5];

              AWAt_raw[20] -= Arow[5] * Arow[5];

              continue;
            }

            numInBounds++;

            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1.0f - alphaX;

            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;

            const s32 y0S32 = Round<s32>(y0);
            const s32 y1S32 = Round<s32>(y1);
            const s32 x0S32 = Round<s32>(x0);

            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);

            f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR,
              alphaY, alphaYinverse, alphaX, alphaXinverse);

#if USE_INTENSITY_NORMALIZATION
            interpolatedPixelF32 = (interpolatedPixelF32 - curMean) * curSigmaInv;
#endif

            // DEBUG:
            /*
            debugStuff[iSample][0] = curSample.xCoordinate;
            debugStuff[iSample][1] = curSample.yCoordinate;
            debugStuff[iSample][2] = curSample.grayvalue;
            debugStuff[iSample][3] = interpolatedPixelF32;
            debugStuff[iSample][4] = curSample.xGradient;
            debugStuff[iSample][5] = curSample.yGradient;
            */

            //const u8 interpolatedPixel = static_cast<u8>(Round(interpolatedPixelF32));

            // This block is the non-interpolation part of the per-sample algorithm
            {
              //CoreTechPrint("(%f,%f) ", xOriginal, yOriginal);

              // This is the only stuff that depends on the current sample
              const f32 templatePixelValue = static_cast<f32>(curSample.grayvalue);

              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);

              /*
              for(s32 i=0; i<6; ++i) {
              debugA[iSample][i] = values[i];
              }
              */

              for(s32 ia=0; ia<6; ia++) {
                b_raw[ia] += Arow[ia] * tGradientValue;
              }
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)

          /*
          if(numInBounds < 16) {
          AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Projective", "Template drifted too far out of image.");
          return RESULT_OK;
          }
          */

          // Get the symmetric entries back out of the "raw" matrix
          symIndex = 0;
          for(s32 ia=0; ia<6; ia++) {
            f32 * restrict AWAt_ia = AWAt.Pointer(ia,0);
            for(s32 ja=ia; ja<6; ja++) {
              AWAt_ia[ja] = AWAt_raw[symIndex++];
            }
            b[0][ia] = b_raw[ia];
          }

          Matrix::MakeSymmetric(AWAt, false);

          //AWAt.Print("New AWAt");
          //b.Print("New b");

          // DEBUG
          //matlab.PutArray(xTransformedArray, "X");
          //matlab.PutArray(yTransformedArray, "Y");
          //matlab.EvalString("hold off, imagesc(img), axis image, hold on, "
          //                    "plot(X,Y, 'rx'); colormap(gray), drawnow");
          /* matlab.PutArray(debugStuff, "debugStuff");
          std::string A_name("A_"); A_name += std::to_string(whichScale);
          matlab.PutArray(debugA, A_name);
          if(whichScale == 0) {
          //matlab.EvalString("desktop, keyboard");
          }
          */

          bool numericalFailure;

          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false, numericalFailure)) != RESULT_OK)
            return lastResult;

          if(numericalFailure){
            AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Projective", "numericalFailure");
            return RESULT_OK;
          }

          /*
          CoreTechPrint("Raw angle update = (%f,%f,%f) degrees, Raw translation udpate = (%f,%f,%f)\n",
          RAD_TO_DEG(b[0][0]), RAD_TO_DEG(b[0][1]), RAD_TO_DEG(b[0][2]),
          b[0][3], b[0][4], b[0][5]);
          */

          //b.Print("New update");

          // Update the 6DoF parameters
          // Note: we are subtracting the update because we're using an _inverse_
          // compositional LK tracking scheme.
          const f32 Kp = this->GetCurrentGain();

          this->params6DoF.angle_x -= Kp * b[0][0];
          this->params6DoF.angle_y -= Kp * b[0][1];
          this->params6DoF.angle_z -= Kp * b[0][2];

          this->params6DoF.translation.x -= Kp * b[0][3];
          this->params6DoF.translation.y -= Kp * b[0][4];
          this->params6DoF.translation.z -= Kp * b[0][5];

          //this->transformation.Update(b, initialImageScaleF32, scratch, Transformations::TRANSFORM_PROJECTIVE);

          // Compute the new homography from the new 6DoF parameters
          this->UpdateTransformation(scratch);

          //this->transformation.get_homography().Print("new transformation");

          // Check if we're done with iterations
#if COMPUTE_CONVERGENCE_FROM_CORNER_CHANGE
          f32 minChange = UpdatePreviousCorners(transformation, previousCorners, scratch);

          if(minChange < convergenceTolerance * scale) {
#else
          const f32 angleConvergenceTolerance = scale*convergenceTolerance_angle;
          const f32 transConvergenceTolerance = scale*convergenceTolerance_distance;

          if(fabs(b[0][0]) < angleConvergenceTolerance &&
            fabs(b[0][1]) < angleConvergenceTolerance &&
            fabs(b[0][2]) < angleConvergenceTolerance &&
            fabs(b[0][3]) < transConvergenceTolerance &&
            fabs(b[0][4]) < transConvergenceTolerance &&
            fabs(b[0][5]) < transConvergenceTolerance)
          {
            /*
            CoreTechPrint("Final params converged at scale %d: angles = (%f,%f,%f) "
            "degrees, translation = (%f,%f,%f)\n",
            whichScale,
            RAD_TO_DEG(this->params6DoF.angle_x),
            RAD_TO_DEG(this->params6DoF.angle_y),
            RAD_TO_DEG(this->params6DoF.angle_z),
            this->params6DoF.translation.x,
            this->params6DoF.translation.y,
            this->params6DoF.translation.z);
            */
#endif

            //snprintf(this->resultMessageBuffer, RESULT_MSG_LENGTH,
            //         "Converged at scale %f with minChange of %f\n", scale, minChange);

            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)

        //snprintf(this->resultMessageBuffer, RESULT_MSG_LENGTH,
        //         "Failed to converge at scale %f with minChange of %f\n", scale, minChange);

        return RESULT_OK;
      } // Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Projective()

      Result LucasKanadeTracker_SampledPlanar6dof::VerifyTrack_Projective(
        const Array<u8> &nextImage,
        const u8 verify_maxPixelDifference,
        s32 &verify_meanAbsoluteDifference,
        s32 &verify_numInBounds,
        s32 &verify_numSimilarPixels,
        MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);
        const s32 verify_maxPixelDifferenceS32 = verify_maxPixelDifference;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const s32 initialImageScaleS32 = baseImageWidth / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const s32 numVerifySamples = this->verificationSamples.get_size();

        const Array<f32> &homography = this->transformation.get_homography();
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
        const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32;
        const f32 h22 = homography[2][2];

        verify_numInBounds = 0;
        verify_numSimilarPixels = 0;
        s32 totalGrayvalueDifference = 0;

        // TODO: make the x and y limits from 1 to end-2

        for(s32 iSample=0; iSample<numVerifySamples; iSample++) {
          const VerifySample curSample = this->verificationSamples[iSample];
          const f32 yOriginal = static_cast<f32>(curSample.yCoordinate) * this->verifyCoordScalar;
          const f32 xOriginal = static_cast<f32>(curSample.xCoordinate) * this->verifyCoordScalar;

          const s32 verificationPixelValue = static_cast<s32>(curSample.grayvalue);

          // TODO: These two could be strength reduced
          const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
          const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

          const f32 normalization = 1.f / (h20*xOriginal + h21*yOriginal + h22);

          const f32 xTransformed = (xTransformedRaw * normalization) + centerOffsetScaled.x;
          const f32 yTransformed = (yTransformedRaw * normalization) + centerOffsetScaled.y;

#if USE_LINEAR_INTERPOLATION_FOR_VERIFICATION
          const f32 x0 = floorf(xTransformed);
          const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;

          const f32 y0 = floorf(yTransformed);
          const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;

          // If out of bounds, continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            continue;
          }

          const f32 alphaX = xTransformed - x0;
          const f32 alphaXinverse = 1 - alphaX;

          const f32 alphaY = yTransformed - y0;
          const f32 alphaYinverse = 1.0f - alphaY;

          const s32 y0S32 = Round<s32>(y0);
          const s32 y1S32 = Round<s32>(y1);
          const s32 x0S32 = Round<s32>(x0);

          const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
          const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);

          // Interpolate the value of this sample in the current image...
          const s32 interpolatedPixelValue = Round<s32>(InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse));

#else // No linear interpolation, just nearest pixel

          if(xTransformed < xyReferenceMin || xTransformed > xReferenceMax ||
            yTransformed < xyReferenceMin || yTransformed > yReferenceMax)
          {
            // Out of bounds
            continue;
          }

          const s32 xS32 = Round<s32>(xTransformed);
          const s32 yS32 = Round<s32>(yTransformed);

          // Lookup nearest pixel value in the current image...
          const u8 * restrict pReference = nextImage.Pointer(yS32, xS32);

          const s32 interpolatedPixelValue = static_cast<s32>(*pReference);

#endif // #if USE_LINEAR_INTERPOLATION_FOR_VERIFICATION

          verify_numInBounds++;

          // ...compare that value to the template sample
          const s32 grayvalueDifference = ABS(interpolatedPixelValue - verificationPixelValue);

          // Keep track of the total difference
          totalGrayvalueDifference += grayvalueDifference;

          if(grayvalueDifference <= verify_maxPixelDifferenceS32) {
            verify_numSimilarPixels++;
          }

#if UPDATE_VERIFICATION_SAMPLES_DURING_TRACKING
          // Now that we're done using it, replace this verification sample
          // with the value we read out.  If tracking is deemed a success by
          // the caller, this will be the new verification value that gets
          // used for the next tracker update.
          this->verificationSamples[iSample].grayvalue = saturate_cast<u8>(interpolatedPixelValue);
#endif
        } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)

        if(verify_numInBounds > 0) {
          verify_meanAbsoluteDifference = totalGrayvalueDifference / verify_numInBounds;
        } else {
          verify_meanAbsoluteDifference = std::numeric_limits<s32>::max();
        }

        return RESULT_OK;
      } // VerifyTrack_Projective()

      Result LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect(const Array<f32> &magnitudeVector, const s32 numBins, const s32 numToSelect, s32 &numSelected, Array<s32> &magnitudeIndexes)
      {
        const f32 maxMagnitude = Matrix::Max<f32>(magnitudeVector);

        const f32 magnitudeIncrement = maxMagnitude / static_cast<f32>(numBins);

        // For each threshold, count the number above the threshold
        // If the number is low enough, copy the appropriate indexes and return

        const s32 numMagnitudes = magnitudeVector.get_size(1);

        const f32 * restrict pMagnitudeVector = magnitudeVector.Pointer(0,0);

        numSelected = 0;

        f32 foundThreshold = -1.0f;
        for(f32 threshold=0; threshold<maxMagnitude; threshold+=magnitudeIncrement) {
          s32 numAbove = 0;
          for(s32 i=0; i<numMagnitudes; i++) {
            if(pMagnitudeVector[i] > threshold) {
              numAbove++;
            }
          }

          if(numAbove <= numToSelect) {
            foundThreshold = threshold;
            break;
          }
        }

        if(foundThreshold < -0.1f) {
          AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect", "Could not find valid threshold");
          magnitudeIndexes.SetZero();
          return RESULT_OK;
        }

        s32 * restrict pMagnitudeIndexes = magnitudeIndexes.Pointer(0,0);

        for(s32 i=0; i<numMagnitudes; i++) {
          if(pMagnitudeVector[i] > foundThreshold) {
            pMagnitudeIndexes[numSelected] = i;
            numSelected++;
          }
        }

        return RESULT_OK;
      } // ApproximateSelect()

      Result LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect(const Array<f32> &magnitudeImage, const s32 numBins, const s32 numRegions, const s32 numToSelect, s32 &numSelected, Array<s32> &magnitudeIndexes)
      {
        //magnitudeImage.Show("magnitudeImage", false);

        // For each threshold, count the number above the threshold
        // If the number is low enough, copy the appropriate indexes and return

        const s32 nrows = magnitudeImage.get_size(0);
        const s32 ncols = magnitudeImage.get_size(1);

        AnkiConditionalErrorAndReturnValue(magnitudeIndexes.get_size(0)*magnitudeIndexes.get_size(1) == nrows*ncols,
                                           RESULT_FAIL_INVALID_SIZE,
                                           "LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect",
                                           "Size of vector does not match size of image");

        numSelected = 0;
        s32 * restrict pMagnitudeIndexes = magnitudeIndexes.Pointer(0,0);

        const s32 regionRows = magnitudeImage.get_size(0) / numRegions;
        const s32 regionCols = magnitudeImage.get_size(1) / numRegions;

        s32 numRegionsLeft = numRegions*numRegions;

        for(s32 i_region=0; i_region<numRegions; ++i_region) {
          const s32 i_min = i_region * regionRows;
          const s32 i_max = i_min + regionRows;

          for(s32 j_region=0; j_region<numRegions; ++j_region) {
            const s32 j_min = j_region*regionCols;
            const s32 j_max = j_min + regionCols;

            ConstArraySlice<f32> currentRegion = magnitudeImage(i_min, i_max-1, j_min, j_max-1);
            const f32 maxMagnitude = Matrix::Max<f32>(currentRegion);
            const f32 minMagnitude = 0.5f*maxMagnitude; // + Matrix::Min<f32>(currentRegion));

            if(maxMagnitude == minMagnitude) {
              // This would result in magnitudeIncrement==0 below
              continue;
            }

            const f32 magnitudeIncrement = (maxMagnitude-minMagnitude) / static_cast<f32>(numBins);

            const s32 numToSelectThisRegion = (numToSelect-numSelected)/numRegionsLeft;
            //const s32 numToSelectThisRegion = numToSelect / (numRegions*numRegions);

            f32 foundThreshold = -1.0f;
            for(f32 threshold=minMagnitude; threshold<maxMagnitude; threshold+=magnitudeIncrement) {
              s32 numAbove = 0;
              for(s32 i=i_min; i<i_max; i++) {
                const f32 * restrict mag_i = magnitudeImage.Pointer(i,0);
                for(s32 j=j_min; j<j_max; j++) {
                  if(mag_i[j] > threshold) {
                    numAbove++;
                  }
                }
              }

              if(numAbove <= numToSelectThisRegion) {
                foundThreshold = threshold;
                break;
              }
            } // for each threshold

            /*
            if(foundThreshold < -0.1f) {
            AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect",
            "Could not find valid threshold for region (%d,%d).", i_region, j_region);
            magnitudeIndexes.SetZero();
            return RESULT_OK;
            }
            */

            if(foundThreshold > 0.f) {
              s32 numSelectedThisRegion = 0;
              for(s32 i=i_min; i<i_max; i++) {
                const f32 * restrict mag_i = magnitudeImage.Pointer(i,0);

                for(s32 j=j_min; j<j_max && numSelectedThisRegion < numToSelectThisRegion; j++) {
                  if(mag_i[j] > foundThreshold) {
                    pMagnitudeIndexes[numSelected++] = i*ncols + j;

                    ++numSelectedThisRegion;
                  }
                }
              }

              //AnkiWarn("ApproximateSelect", "Finished region (%d,%d), have %d selected", i_region, j_region, numSelected);
            } // if(foundThreshold > 0.f)

            --numRegionsLeft;
          } // for j_region
        } // for i_region

        return RESULT_OK;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
