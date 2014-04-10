/**
 File: lucasKanade_SampledPlanar6dof.cpp
 Author: Andrew Stein
 Created: 2014-04-04
 
 Copyright Anki, Inc. 2014
 For internal use only. No part of this code may be used without a signed 
 non-disclosure agreement with Anki, inc.
 **/

#include "anki/vision/robot/lucasKanade.h"
#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/interpolate.h"
#include "anki/common/robot/arrayPatterns.h"
#include "anki/common/robot/find.h"
#include "anki/common/robot/benchmarking_c.h"
#include "anki/common/robot/draw.h"
#include "anki/common/robot/comparisons.h"
#include "anki/common/robot/errorHandling.h"

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/imageProcessing.h"
#include "anki/vision/robot/transformations.h"
#include "anki/vision/robot/perspectivePoseEstimation.h"

//#define SEND_BINARY_IMAGES_TO_MATLAB

#define USE_OPENCV_ITERATIVE_POSE_INIT 0

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
      : LucasKanadeTracker_Generic(maxSupportedTransformType)
      {
      }
      
      LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof(
                                                                                 const Array<u8> &templateImage,
                                                                                 const Quadrilateral<f32> &templateQuadIn,
                                                                                 const f32 scaleTemplateRegionPercent,
                                                                                 const s32 numPyramidLevels,
                                                                                 const Transformations::TransformType transformType,
                                                                                 const s32 maxSamplesAtBaseLevel,
                                                                                 const f32 focalLength_x,
                                                                                 const f32 focalLength_y,
                                                                                 const f32 camCenter_x,
                                                                                 const f32 camCenter_y,
                                                                                 const f32 templateWidth_mm,
                                                                                 MemoryStack ccmMemory,
                                                                                 MemoryStack &onchipScratch,
                                                                                 MemoryStack offchipScratch)
      : LucasKanadeTracker_Generic(Transformations::TRANSFORM_PROJECTIVE, templateImage, templateQuadIn, scaleTemplateRegionPercent, numPyramidLevels, transformType, onchipScratch)
      {
        // Allocate all the
        
        // Initialize calibration data
        this->focalLength_x = focalLength_x;
        this->focalLength_y = focalLength_y;
        this->camCenter_x   = camCenter_x;
        this->camCenter_y   = camCenter_y;

        // Create a canonical 3D template to use.
        Point3<f32> template3d[4];
        const f32 templateHalfWidth = templateWidth_mm * 0.5f;
        template3d[0] = Point3<f32>(-templateHalfWidth, -templateHalfWidth, 0.f);
        template3d[1] = Point3<f32>(-templateHalfWidth,  templateHalfWidth, 0.f);
        template3d[2] = Point3<f32>( templateHalfWidth, -templateHalfWidth, 0.f);
        template3d[3] = Point3<f32>( templateHalfWidth,  templateHalfWidth, 0.f);
        Array<f32> R = Array<f32>(3,3,onchipScratch); // TODO: which scratch?
        
        // No matter the orientation of the incoming quad, we are always
        // assuming the canonical 3D marker created above is vertically oriented.
        // So, make this one vertically oriented too.
        // TODO: better way to accomplish this?  Feed in un-reordered corners from detected marker?
        Quadrilateral<f32> clockwiseQuad = templateQuadIn.ComputeClockwiseCorners();
        Quadrilateral<f32> templateQuad(clockwiseQuad[0], clockwiseQuad[3],
                                        clockwiseQuad[1], clockwiseQuad[2]);
        
        
        // Compute the initial pose from the calibration, and the known physical
        // size of the template.  This gives us R matrix and T vector.
        // TODO: is single precision enough for the P3P::computePose call here?
#if USE_OPENCV_ITERATIVE_POSE_INIT
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
          
          cv::Mat distortionCoeffs; // TODO: currently empty, use radial distoration?
          cv::solvePnP(cvObjPoints, cvImagePoints,
                       calibMatrix.get_CvMat_(), distortionCoeffs,
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
        
        P3P::computePose(templateQuad,
                         template3d[0], template3d[1], template3d[2], template3d[3],
                         this->focalLength_x, this->focalLength_y,
                         this->camCenter_x,   this->camCenter_y,
                         R, this->params6DoF.translation,
                         onchipScratch);  // TODO: which scratch?
        
#endif // #if USE_OPENCV_ITERATIVE_POSE_INIT
        
        /*
        R.Print("Initial R for tracker:", 0,3,0,3);
        printf("Initial T for tracker: ");
        this->params6DoF.translation.Print();
        printf("\n");
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
        Quadrilateral<f32> initCorners(Point2f(template3d[0].x, template3d[0].y),
                                       Point2f(template3d[1].x, template3d[1].y),
                                       Point2f(template3d[2].x, template3d[2].y),
                                       Point2f(template3d[3].x, template3d[3].y));
        
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
        const f32 sx = sinf(params6DoF.angle_x);
        const f32 sy = sinf(params6DoF.angle_y);
        const f32 sz = sinf(params6DoF.angle_z);
        const f32 cx = cosf(params6DoF.angle_x);
        const f32 cy = cosf(params6DoF.angle_y);
        const f32 cz = cosf(params6DoF.angle_z);
        
        //const f32 dr11_dthetaX = 0.f;
        const f32 dr12_dthetaX = -sx*sz + cx*sy*cz;
        //const f32 dr21_dthetaX = 0.f;
        const f32 dr22_dthetaX = -sx*cz - cx*sy*sz;
        //const f32 dr31_dthetaX = 0.f;
        const f32 dr32_dthetaX = -cx*cy;
        
        const f32 dr11_dthetaY = -sy*cz;
        const f32 dr12_dthetaY = sx*cy*cz;
        const f32 dr21_dthetaY = sy*sz;
        const f32 dr22_dthetaY = -sx*cy*sz;
        const f32 dr31_dthetaY = cy;
        const f32 dr32_dthetaY = sx*sy;
        
        const f32 dr11_dthetaZ = -cy*sz;
        const f32 dr12_dthetaZ = cx*cz - sx*sy*sz;
        const f32 dr21_dthetaZ = -cy*cz;
        const f32 dr22_dthetaZ = -cx*sz - sx*sy*cz;
        //const f32 dr31_dthetaZ = 0.f;
        //const f32 dr32_dthetaZ = 0.f;
        
        
        const s32 numSelectBins = 20;
        
        Result lastResult;
        
        BeginBenchmark("LucasKanadeTracker_SampledPlanar6dof");
        
        this->templateSamplePyramid = FixedLengthList<FixedLengthList<TemplateSample> >(numPyramidLevels, onchipScratch);
        this->jacobianSamplePyramid = FixedLengthList<FixedLengthList<JacobianSample> >(numPyramidLevels, onchipScratch);
        
        for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
          const f32 scale = static_cast<f32>(1 << iScale);
          
          // Coordinates for this tracker are 3D plane coordinates
          const f32 halfWidth = scaleTemplateRegionPercent*templateHalfWidth;
          Meshgrid<f32> curTemplateCoordinates = Meshgrid<f32>(Linspace(-halfWidth, halfWidth,
                                                                        static_cast<s32>(FLT_FLOOR(this->templateRegionWidth/scale))),
                                                               Linspace(-halfWidth, halfWidth,
                                                                        static_cast<s32>(FLT_FLOOR(this->templateRegionHeight/scale))));
          
          // Half the sample at each subsequent level (not a quarter)
          const s32 maxPossibleLocations = curTemplateCoordinates.get_numElements();
          const s32 curMaxSamples = MIN(maxPossibleLocations, maxSamplesAtBaseLevel >> iScale);
          
          this->templateSamplePyramid[iScale] = FixedLengthList<TemplateSample>(curMaxSamples, onchipScratch);
          this->templateSamplePyramid[iScale].set_size(curMaxSamples);
          
          this->jacobianSamplePyramid[iScale] = FixedLengthList<JacobianSample>(curMaxSamples, onchipScratch);
          this->jacobianSamplePyramid[iScale].set_size(curMaxSamples);
        }
        
        //
        // Temporary allocations below this point
        //
        {
          // Everything allocated using offchipScratch above will survive
          //PUSH_MEMORY_STACK(offchipScratch);
          
          // This section is based off lucasKanade_Fast, except uses f32 in offchip instead of integer types in onchip
          
          FixedLengthList<Meshgrid<f32> > templateCoordinates = FixedLengthList<Meshgrid<f32> >(numPyramidLevels, offchipScratch);
          FixedLengthList<Array<f32> > templateImagePyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);
          FixedLengthList<Array<f32> > templateImageXGradientPyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);
          FixedLengthList<Array<f32> > templateImageYGradientPyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);
          FixedLengthList<Array<f32> > templateImageSquaredGradientMagnitudePyramid = FixedLengthList<Array<f32> >(numPyramidLevels, offchipScratch);
          
          templateCoordinates.set_size(numPyramidLevels);
          templateImagePyramid.set_size(numPyramidLevels);
          templateImageXGradientPyramid.set_size(numPyramidLevels);
          templateImageYGradientPyramid.set_size(numPyramidLevels);
          templateImageSquaredGradientMagnitudePyramid.set_size(numPyramidLevels);
          
          AnkiConditionalErrorAndReturn(templateImagePyramid.IsValid() && templateImageXGradientPyramid.IsValid() && templateImageYGradientPyramid.IsValid() && templateCoordinates.IsValid() && templateImageSquaredGradientMagnitudePyramid.IsValid(),
                                        "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "Could not allocate pyramid lists");
          
          // Allocate the memory for all the images
          for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
            const f32 scale = static_cast<f32>(1 << iScale);
            
            // Note that template coordinates for this tracker are actually
            // coordinates on the 3d template, which get mapped into the image
            // by the homography
            const f32 halfWidth = scaleTemplateRegionPercent*templateHalfWidth;
            templateCoordinates[iScale] = Meshgrid<f32>(
                                                        Linspace(-halfWidth, halfWidth,
                                                                 static_cast<s32>(FLT_FLOOR(this->templateRegionWidth/scale))),
                                                        Linspace(-halfWidth, halfWidth,
                                                                 static_cast<s32>(FLT_FLOOR(this->templateRegionHeight/scale))));
            
            const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();
            
            templateImagePyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);
            templateImageXGradientPyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);
            templateImageYGradientPyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);
            templateImageSquaredGradientMagnitudePyramid[iScale] = Array<f32>(numPointsY, numPointsX, offchipScratch);
            
            AnkiConditionalErrorAndReturn(templateImagePyramid[iScale].IsValid() && templateImageXGradientPyramid[iScale].IsValid() && templateImageYGradientPyramid[iScale].IsValid() && templateImageSquaredGradientMagnitudePyramid[iScale].IsValid(),
                                          "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "Could not allocate pyramid images");
          }
          
          // Sample all levels of the pyramid images
          for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
            if((lastResult = Interp2_Projective<u8,f32>(templateImage, templateCoordinates[iScale], transformation.get_homography(), this->transformation.get_centerOffset(initialImageScaleF32), templateImagePyramid[iScale], INTERPOLATE_LINEAR)) != RESULT_OK) {
              AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "Interp2_Projective failed with code 0x%x", lastResult);
              return;
            }
            
            //cv::imshow("templateImagePyramid[iScale]", templateImagePyramid[iScale].get_CvMat_());
            //cv::waitKey();
          }
          
          // Compute the spatial derivatives
          // TODO: compute without borders?
          for(s32 iScale=0; iScale<numPyramidLevels; iScale++) {
            PUSH_MEMORY_STACK(offchipScratch);
            PUSH_MEMORY_STACK(onchipScratch);
            
            const s32 numPointsY = templateCoordinates[iScale].get_yGridVector().get_size();
            const s32 numPointsX = templateCoordinates[iScale].get_xGridVector().get_size();
            
            if((lastResult = ImageProcessing::ComputeXGradient<f32,f32,f32>(templateImagePyramid[iScale], templateImageXGradientPyramid[iScale])) != RESULT_OK) {
              AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "ComputeXGradient failed with code 0x%x", lastResult);
              return;
            }
            
            if((lastResult = ImageProcessing::ComputeYGradient<f32,f32,f32>(templateImagePyramid[iScale], templateImageYGradientPyramid[iScale])) != RESULT_OK) {
              AnkiError("LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "ComputeYGradient failed with code 0x%x", lastResult);
              return;
            }
            
            //templateImageXGradientPyramid[iScale].Show("X Gradient", false);
            //templateImageYGradientPyramid[iScale].Show("Y Gradient", true);
            
            // Using the computed gradients, find a set of the max values, and store them
            
            Array<f32> tmpMagnitude(numPointsY, numPointsX, offchipScratch);
            
            Matrix::DotMultiply<f32,f32,f32>(templateImageXGradientPyramid[iScale], templateImageXGradientPyramid[iScale], tmpMagnitude);
            Matrix::DotMultiply<f32,f32,f32>(templateImageYGradientPyramid[iScale], templateImageYGradientPyramid[iScale], templateImageSquaredGradientMagnitudePyramid[iScale]);
            Matrix::Add<f32,f32,f32>(tmpMagnitude, templateImageSquaredGradientMagnitudePyramid[iScale], templateImageSquaredGradientMagnitudePyramid[iScale]);
            
            //Matrix::Sqrt<f32,f32,f32>(templateImageSquaredGradientMagnitudePyramid[iScale], templateImageSquaredGradientMagnitudePyramid[iScale]);
            
            //cv::imshow("templateImageSquaredGradientMagnitude[iScale]", templateImageSquaredGradientMagnitudePyramid[iScale].get_CvMat_());
            //cv::waitKey();
            
            Array<f32> magnitudeVector = Matrix::Vectorize<f32,f32>(false, templateImageSquaredGradientMagnitudePyramid[iScale], offchipScratch);
            Array<u16> magnitudeIndexes = Array<u16>(1, numPointsY*numPointsX, offchipScratch);
            
            AnkiConditionalErrorAndReturn(magnitudeVector.IsValid() && magnitudeIndexes.IsValid(),
                                          "LucasKanadeTracker_SampledPlanar6dof::LucasKanadeTracker_SampledPlanar6dof", "Out of memory");
            
            // Really slow
            //const f32 t0 = GetTime();
            //Matrix::Sort<f32>(magnitudeVector, magnitudeIndexes, 1, false);
            //const f32 t1 = GetTime();
            
            const s32 numSamples = this->templateSamplePyramid[iScale].get_size();
            s32 numSelected;
            LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect(magnitudeVector, numSelectBins, numSamples, numSelected, magnitudeIndexes);
            
            //const f32 t2 = GetTime();
            //printf("%f %f\n", t1-t0, t2-t1);
            
            if(numSelected == 0) {
              return;
            }
            
            //{
            //  Matlab matlab(false);
            //  matlab.PutArray(magnitudeVector, "magnitudeVector");
            //  matlab.PutArray(magnitudeIndexes, "magnitudeIndexes");
            //}
            
            Array<f32> yCoordinatesVector = templateCoordinates[iScale].EvaluateY1(false, offchipScratch);
            Array<f32> xCoordinatesVector = templateCoordinates[iScale].EvaluateX1(false, offchipScratch);
            
            Array<f32> yGradientVector = Matrix::Vectorize<f32,f32>(false, templateImageYGradientPyramid[iScale], offchipScratch);
            Array<f32> xGradientVector = Matrix::Vectorize<f32,f32>(false, templateImageXGradientPyramid[iScale], offchipScratch);
            Array<f32> grayscaleVector = Matrix::Vectorize<f32,f32>(false, templateImagePyramid[iScale], offchipScratch);
            
            const f32 * restrict pYCoordinates = yCoordinatesVector.Pointer(0,0);
            const f32 * restrict pXCoordinates = xCoordinatesVector.Pointer(0,0);
            const f32 * restrict pYGradientVector = yGradientVector.Pointer(0,0);
            const f32 * restrict pXGradientVector = xGradientVector.Pointer(0,0);
            const f32 * restrict pGrayscaleVector = grayscaleVector.Pointer(0,0);
            const u16 * restrict pMagnitudeIndexes = magnitudeIndexes.Pointer(0,0);
            
            const f32 h00 = initialHomography[0][0];
            const f32 h01 = initialHomography[0][1];
            const f32 h02 = initialHomography[0][2] / initialImageScaleF32;
            
            const f32 h10 = initialHomography[1][0];
            const f32 h11 = initialHomography[1][1];
            const f32 h12 = initialHomography[1][2] / initialImageScaleF32;
            
            const f32 h20 = initialHomography[2][0] * initialImageScaleF32;
            const f32 h21 = initialHomography[2][1] * initialImageScaleF32;
            const f32 h22 = initialHomography[2][2];
            
            TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[iScale].Pointer(0);
            JacobianSample * restrict pJacobianSamplePyramid = this->jacobianSamplePyramid[iScale].Pointer(0);
            
            for(s32 iSample=0; iSample<numSamples; iSample++){
              const s32 curIndex = pMagnitudeIndexes[iSample];
              
              TemplateSample curTemplateSample;
              curTemplateSample.xCoordinate = pXCoordinates[curIndex];
              curTemplateSample.yCoordinate = pYCoordinates[curIndex];
              curTemplateSample.xGradient   = pXGradientVector[curIndex];
              curTemplateSample.yGradient   = pYGradientVector[curIndex];
              curTemplateSample.grayvalue   = pGrayscaleVector[curIndex];
              
              pTemplateSamplePyramid[iSample] = curTemplateSample;
              
              // Everything below here is about filling in the Jacobian info
              // for this sample
              
              JacobianSample curJacobianSample;
              
              const f32 xOriginal = curTemplateSample.xCoordinate;
              const f32 yOriginal = curTemplateSample.yCoordinate;
              
              // TODO: These two could be strength reduced
              const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
              const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;
              
              const f32 normalization = h20*xOriginal + h21*yOriginal + h22;
              
              const f32 invNorm = 1.f / normalization;
              const f32 invNormSq = invNorm *invNorm;
              
              curJacobianSample.dWu_dtx = this->focalLength_x * invNorm;
              //curJacobianSample.dWu_dty = 0.f;
              curJacobianSample.dWu_dtz = -xTransformedRaw * invNormSq;
              
              //curJacobianSample.dWv_dtx = 0.f;
              curJacobianSample.dWv_dty = this->focalLength_y * invNorm;
              curJacobianSample.dWv_dtz = -yTransformedRaw * invNormSq;
              
              const f32 r1thetaXterm = /*dr11_dthetaX*xOriginal +*/ dr12_dthetaX*yOriginal;
              const f32 r1thetaYterm = dr11_dthetaY*xOriginal + dr12_dthetaY*yOriginal;
              const f32 r1thetaZterm = dr11_dthetaZ*xOriginal + dr12_dthetaZ*yOriginal;
              
              const f32 r2thetaXterm = /*dr21_dthetaX*xOriginal + */dr22_dthetaX*yOriginal;
              const f32 r2thetaYterm = dr21_dthetaY*xOriginal + dr22_dthetaY*yOriginal;
              const f32 r2thetaZterm = dr21_dthetaZ*xOriginal + dr22_dthetaZ*yOriginal;
              
              const f32 r3thetaXterm = /*dr31_dthetaX*xOriginal + */dr32_dthetaX*yOriginal;
              const f32 r3thetaYterm = dr31_dthetaY*xOriginal + dr32_dthetaY*yOriginal;
              //const f32 r3thetaZterm = dr31_dthetaZ*xOriginal + dr32_dthetaZ*yOriginal;
              
              curJacobianSample.dWu_dthetaX = (this->focalLength_x*normalization*r1thetaXterm -
                                               r3thetaXterm*xTransformedRaw) * invNormSq;
              
              curJacobianSample.dWu_dthetaY = (this->focalLength_x*normalization*r1thetaYterm -
                                               r3thetaYterm*xTransformedRaw) * invNormSq;
              
              curJacobianSample.dWu_dthetaZ = (this->focalLength_x*normalization*r1thetaZterm /*-
                                               r3thetaZterm*xTransformedRaw*/) * invNormSq;
              
              
              curJacobianSample.dWv_dthetaX = (this->focalLength_y*normalization*r2thetaXterm -
                                               r3thetaXterm*yTransformedRaw) * invNormSq;
              
              curJacobianSample.dWv_dthetaY = (this->focalLength_y*normalization*r2thetaYterm -
                                               r3thetaYterm*yTransformedRaw) * invNormSq;
              
              curJacobianSample.dWv_dthetaZ = (this->focalLength_y*normalization*r2thetaZterm /*-
                                               r3thetaZterm*yTransformedRaw*/) * invNormSq;
              
              pJacobianSamplePyramid[iSample] = curJacobianSample;

            }
          }
        } // PUSH_MEMORY_STACK(fastMemory);
        
        this->isValid = true;
        
        EndBenchmark("LucasKanadeTracker_SampledPlanar6dof");
      }
      
      
      // Fill a rotation matrix according to the current tracker angles
      // R should already be allocated to be 3x3
      Result LucasKanadeTracker_SampledPlanar6dof::get_rotationMatrix(Array<f32>& R,
                                                                      bool skipLastColumn) const
      {
        AnkiConditionalErrorAndReturnValue(R.get_size(0)==3 && R.get_size(1)==3, RESULT_FAIL_INVALID_SIZE,
                                           "LucasKanadeTracker_SampledPlanar6dof::get_rotationMatrix",
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
      } // get_rotationMatrix()
      
      // Set the tracker angles by extracting Euler angles from the given
      // rotation matrix
      Result LucasKanadeTracker_SampledPlanar6dof::set_rotationAnglesFromMatrix(const Array<f32>& R)
      {
        AnkiConditionalErrorAndReturnValue(R.get_size(0)==3 && R.get_size(1)==3, RESULT_FAIL_INVALID_SIZE,
                                      "LucasKanadeTracker_SampledPlanar6dof::set_rotationAnglesFromMatrix",
                                      "R should be a 3x3 matrix.");

        // Extract Euler angles from given rotation matrix:
        if(fabs(1.f - R[2][0]) < 1e-6f) { // is R(2,0) == 1
          this->params6DoF.angle_z = 0.f;
          if(R[2][0] > 0) { // R(2,0) = +1
            this->params6DoF.angle_y = M_PI_2;
            this->params6DoF.angle_x = atan2_acc(R[0][1], R[1][1]);
          } else { // R(2,0) = -1
            this->params6DoF.angle_y = -M_PI_2;
            this->params6DoF.angle_x = atan2_acc(-R[0][1], R[1][1]);
          }
        } else {
          this->params6DoF.angle_y = asinf(R[2][0]);
          const f32 inv_cy = 1.f / cosf(this->params6DoF.angle_y);
          this->params6DoF.angle_x = atan2_acc(-R[2][1]*inv_cy, R[2][2]*inv_cy);
          this->params6DoF.angle_z = atan2_acc(-R[1][0]*inv_cy, R[0][0]*inv_cy);
        }
        
        return RESULT_OK;
        
      } // set_rotationMatrix()
      
      // Retrieve/update the current translation estimates of the tracker
      const Point3<f32>& LucasKanadeTracker_SampledPlanar6dof::get_translation() const
      {
        return this->params6DoF.translation;
      } // get_translation()
      
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
        this->get_rotationMatrix(newHomography, true);
        
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
#ifndef ANKICORETECH_EMBEDDED_USE_OPENCV
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
            
            const s32 curY = RoundS32(curTemplateSample.yCoordinate + centerOffset.y);
            const s32 curX = RoundS32(curTemplateSample.xCoordinate + centerOffset.x);
            
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
          
          cv::imshow(windowNameTotal, image.get_CvMat_());
        }
        
        if(waitForKeypress)
          cv::waitKey();
        
        return RESULT_OK;
#endif // #ifndef ANKICORETECH_EMBEDDED_USE_OPENCV ... #else
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
                                                               const f32 convergenceTolerance,
                                                               const u8 verify_maxPixelDifference,
                                                               bool &verify_converged,
                                                               s32 &verify_meanAbsoluteDifference,
                                                               s32 &verify_numInBounds,
                                                               s32 &verify_numSimilarPixels,
                                                               MemoryStack scratch)
      {
        Result lastResult;
        
        for(s32 iScale=numPyramidLevels-1; iScale>=0; iScale--) {
          verify_converged = false;
          
          BeginBenchmark("UpdateTrack.refineTranslation");
          if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, Transformations::TRANSFORM_TRANSLATION, verify_converged, scratch)) != RESULT_OK)
            return lastResult;
          EndBenchmark("UpdateTrack.refineTranslation");
          
          if(this->transformation.get_transformType() != Transformations::TRANSFORM_TRANSLATION) {
            BeginBenchmark("UpdateTrack.refineOther");
            if((lastResult = IterativelyRefineTrack(nextImage, maxIterations, iScale, convergenceTolerance, this->transformation.get_transformType(), verify_converged, scratch)) != RESULT_OK)
              return lastResult;
            EndBenchmark("UpdateTrack.refineOther");
          }
        } // for(s32 iScale=numPyramidLevels; iScale>=0; iScale--)
        
        lastResult = this->VerifyTrack_Projective(nextImage, verify_maxPixelDifference, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels, scratch);
        
        return lastResult;
      }
      
      Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, const Transformations::TransformType curTransformType, bool &verify_converged, MemoryStack scratch)
      {
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);
        
        AnkiConditionalErrorAndReturnValue(this->IsValid() == true,
                                           RESULT_FAIL, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "This object is not initialized");
        
        AnkiConditionalErrorAndReturnValue(nextImage.IsValid(),
                                           RESULT_FAIL_INVALID_OBJECT, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "nextImage is not valid");
        
        AnkiConditionalErrorAndReturnValue(maxIterations > 0 && maxIterations < 1000,
                                           RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "maxIterations must be greater than zero and less than 1000");
        
        AnkiConditionalErrorAndReturnValue(whichScale >= 0 && whichScale < this->numPyramidLevels,
                                           RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "whichScale is invalid");
        
        AnkiConditionalErrorAndReturnValue(convergenceTolerance > 0.0f,
                                           RESULT_FAIL_INVALID_PARAMETERS, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "convergenceTolerance must be greater than zero");
        
        AnkiConditionalErrorAndReturnValue(nextImageHeight == templateImageHeight && nextImageWidth == templateImageWidth,
                                           RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "nextImage must be the same size as the template");
        
        const s32 initialImageScaleS32 = BASE_IMAGE_WIDTH / nextImageWidth;
        const s32 initialImagePowerS32 = Log2u32(static_cast<u32>(initialImageScaleS32));
        
        AnkiConditionalErrorAndReturnValue(((1<<initialImagePowerS32)*nextImageWidth) == BASE_IMAGE_WIDTH,
                                           RESULT_FAIL_INVALID_SIZE, "LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack", "The templateImage must be a power of two smaller than BASE_IMAGE_WIDTH");
        
        if(curTransformType == Transformations::TRANSFORM_TRANSLATION) {
          return IterativelyRefineTrack_Translation(nextImage, maxIterations, whichScale, convergenceTolerance, verify_converged, scratch);
        } else if(curTransformType == Transformations::TRANSFORM_AFFINE) {
          return IterativelyRefineTrack_Affine(nextImage, maxIterations, whichScale, convergenceTolerance, verify_converged, scratch);
        } else if(curTransformType == Transformations::TRANSFORM_PROJECTIVE) {
          return IterativelyRefineTrack_Projective(nextImage, maxIterations, whichScale, convergenceTolerance, verify_converged, scratch);
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
          return 0.25f;
        }
      }
      
      Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &verify_converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);
        
        Result lastResult;
        
        Array<f32> AWAt(3, 3, scratch);
        Array<f32> b(1, 3, scratch);
        
        f32 &AWAt00 = AWAt[0][0];
        f32 &AWAt01 = AWAt[0][1];
        f32 &AWAt02 = AWAt[0][2];
        //f32 &AWAt10 = AWAt[1][0];
        f32 &AWAt11 = AWAt[1][1];
        f32 &AWAt12 = AWAt[1][2];
        f32 &AWAt22 = AWAt[2][2];
        
        f32 &b0 = b[0][0];
        f32 &b1 = b[0][1];
        f32 &b2 = b[0][2];
        
        verify_converged = false;
        
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);
        
        const f32 scale = static_cast<f32>(1 << whichScale);
        
        const s32 initialImageScaleS32 = BASE_IMAGE_WIDTH / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);
        
        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);
        
        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);
        
        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);
        
        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }
        
        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;
        
        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);
        const JacobianSample * restrict pJacobianSamplePyramid = this->jacobianSamplePyramid[whichScale].Pointer(0);
        
        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);
        
        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32;
          const f32 h22 = homography[2][2];
          
          AWAt.SetZero();
          b.SetZero();
          
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
            
            const f32 x0 = FLT_FLOOR(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;
            
            const f32 y0 = FLT_FLOOR(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;
            
            // If out of bounds, continue
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              continue;
            }
            
            numInBounds++;
            
            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1 - alphaX;
            
            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;
            
            const s32 y0S32 = RoundS32(y0);
            const s32 y1S32 = RoundS32(y1);
            const s32 x0S32 = RoundS32(x0);
            
            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);
            
            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);
            
            const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);
            
            const JacobianSample curJacobianSample = pJacobianSamplePyramid[iSample];
            
            //const u8 interpolatedPixel = static_cast<u8>(Round(interpolatedPixelF32));
            
            // This block is the non-interpolation part of the per-sample algorithm
            {
              const f32 templatePixelValue = curSample.grayvalue;
              const f32 xGradientValue = scaleOverFiveTen * curSample.xGradient;
              const f32 yGradientValue = scaleOverFiveTen * curSample.yGradient;
              
              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);
              
              const f32 dWu_dtx = curJacobianSample.dWu_dtx;
              //const f32 dWv_dtx = curJacobianSample.dWv_dtx;  // always zero
              //const f32 dWu_dty = curJacobianSample.dWu_dty;  // always zero
              const f32 dWv_dty = curJacobianSample.dWv_dty;
              const f32 dWu_dtz = curJacobianSample.dWu_dtz;
              const f32 dWv_dtz = curJacobianSample.dWv_dtz;
              
              const f32 A0 = xGradientValue * dWu_dtx;
              const f32 A1 = yGradientValue * dWv_dty;
              const f32 A2 = xGradientValue*dWu_dtz + yGradientValue*dWv_dtz;
              
              //AWAt
              //  b
              AWAt00 += A0*A0;
              AWAt01 += A0*A1;
              AWAt02 += A0*A2;
              AWAt11 += A1*A1;
              AWAt12 += A1*A2;
              AWAt22 += A2*A2;
              
              b0 += A0 * tGradientValue;
              b1 += A1 * tGradientValue;
              b2 += A2 * tGradientValue;
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)
          
          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation", "Template drifted too far out of image.");
            return RESULT_OK;
          }
          
          Matrix::MakeSymmetric(AWAt, false);
          
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
          //const f32 minChange = UpdatePreviousCorners(transformation, previousCorners, scratch);
          
          //if(minChange < convergenceTolerance * scale)
          // TODO: make these parameters
          //const f32 angleConvergenceTolerance = scale*DEG_TO_RAD(0.1f);
          const f32 transConvergenceTolerance = scale*0.25f;
          
          if(fabs(b[0][0]) < transConvergenceTolerance &&
             fabs(b[0][1]) < transConvergenceTolerance &&
             fabs(b[0][2]) < transConvergenceTolerance)
          {
            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)
        
        return RESULT_OK;
      } // Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Translation()
      
      
      Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Affine(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &verify_converged, MemoryStack scratch)
      {
        /* Not sure this makes sense for the Planar6dof tracker
       
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);
        
        Result lastResult;
        
        Array<f32> AWAt(6, 6, scratch);
        Array<f32> b(1, 6, scratch);
        
        // These addresses should be known at compile time, so should be faster
        f32 AWAt_raw[6][6];
        f32 b_raw[6];
        
        verify_converged = false;
        
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);
        
        const f32 scale = static_cast<f32>(1 << whichScale);
        
        const s32 initialImageScaleS32 = BASE_IMAGE_WIDTH / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);
        
        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);
        
        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);
        
        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);
        
        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }
        
        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;
        
        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);
        
        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);
        
        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32;
          const f32 h22 = homography[2][2];
          
          //AWAt.SetZero();
          //b.SetZero();
          
          for(s32 ia=0; ia<6; ia++) {
            for(s32 ja=0; ja<6; ja++) {
              AWAt_raw[ia][ja] = 0;
            }
            b_raw[ia] = 0;
          }
          
          s32 numInBounds = 0;
          
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
            
            const f32 x0 = FLT_FLOOR(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;
            
            const f32 y0 = FLT_FLOOR(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;
            
            // If out of bounds, continue
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              continue;
            }
            
            numInBounds++;
            
            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1 - alphaX;
            
            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;
            
            const s32 y0S32 = RoundS32(y0);
            const s32 y1S32 = RoundS32(y1);
            const s32 x0S32 = RoundS32(x0);
            
            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);
            
            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);
            
            const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);
            
            //const u8 interpolatedPixel = static_cast<u8>(Round(interpolatedPixelF32));
            
            // This block is the non-interpolation part of the per-sample algorithm
            {
              const f32 templatePixelValue = curSample.grayvalue;
              const f32 xGradientValue = scaleOverFiveTen * curSample.xGradient;
              const f32 yGradientValue = scaleOverFiveTen * curSample.yGradient;
              
              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);
              
              //printf("%f ", xOriginal);
              const f32 values[6] = {
                xOriginal * xGradientValue,
                yOriginal * xGradientValue,
                xGradientValue,
                xOriginal * yGradientValue,
                yOriginal * yGradientValue,
                yGradientValue};
              
              //for(s32 ia=0; ia<6; ia++) {
              //  printf("%f ", values[ia]);
              //}
              //printf("\n");
              
              //f32 AWAt_raw[6][6];
              //f32 b_raw[6];
              for(s32 ia=0; ia<6; ia++) {
                for(s32 ja=ia; ja<6; ja++) {
                  AWAt_raw[ia][ja] += values[ia] * values[ja];
                }
                b_raw[ia] += values[ia] * tGradientValue;
              }
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)
          
          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Affine", "Template drifted too far out of image.");
            return RESULT_OK;
          }
          
          for(s32 ia=0; ia<6; ia++) {
            for(s32 ja=ia; ja<6; ja++) {
              AWAt[ia][ja] = AWAt_raw[ia][ja];
            }
            b[0][ia] = b_raw[ia];
          }
          
          Matrix::MakeSymmetric(AWAt, false);
          
          //AWAt.Print("New AWAt");
          //b.Print("New b");
          
          bool numericalFailure;
          
          if((lastResult = Matrix::SolveLeastSquaresWithCholesky(AWAt, b, false, numericalFailure)) != RESULT_OK)
            return lastResult;
          
          if(numericalFailure){
            AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Affine", "numericalFailure");
            return RESULT_OK;
          }
          
          //b.Print("New update");
          
          this->transformation.Update(b, initialImageScaleF32, scratch, Transformations::TRANSFORM_AFFINE);
          
          //this->transformation.get_homography().Print("new transformation");
          
          // Check if we're done with iterations
          const f32 minChange = UpdatePreviousCorners(transformation, previousCorners, scratch);
          
          if(minChange < convergenceTolerance) {
            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)
        */
        return RESULT_FAIL;
      } // Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Affine()
      
      
      Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Projective(const Array<u8> &nextImage, const s32 maxIterations, const s32 whichScale, const f32 convergenceTolerance, bool &verify_converged, MemoryStack scratch)
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);
        
        Result lastResult;
        
        Array<f32> AWAt(6, 6, scratch);
        Array<f32> b(1, 6, scratch);
        
        // These addresses should be known at compile time, so should be faster
        f32 AWAt_raw[6][6];
        f32 b_raw[6];
        
        verify_converged = false;
        
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);
        
        const f32 scale = static_cast<f32>(1 << whichScale);
        
        const s32 initialImageScaleS32 = BASE_IMAGE_WIDTH / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);
        
        const f32 oneOverTwoFiftyFive = 1.0f / 255.0f;
        const f32 scaleOverFiveTen = scale / (2.0f*255.0f);
        
        //const Point<f32>& centerOffset = this->transformation.get_centerOffset();
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);
        
        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);
        
        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }
        
        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;
        
        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);
        const JacobianSample * restrict pJacobianSamplePyramid = this->jacobianSamplePyramid[whichScale].Pointer(0);
        
        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);
        
        for(s32 iteration=0; iteration<maxIterations; iteration++) {
          const Array<f32> &homography = this->transformation.get_homography();
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
          const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32;
          const f32 h22 = homography[2][2];
          
          //AWAt.SetZero();
          //b.SetZero();
          
          for(s32 ia=0; ia<6; ia++) {
            for(s32 ja=0; ja<6; ja++) {
              AWAt_raw[ia][ja] = 0;
            }
            b_raw[ia] = 0;
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
            
            const f32 normalization = h20*xOriginal + h21*yOriginal + h22;
            
            const f32 xTransformed = (xTransformedRaw / normalization) + centerOffsetScaled.x;
            const f32 yTransformed = (yTransformedRaw / normalization) + centerOffsetScaled.y;
            
            // DEBUG!
            //xTransformedArray[0][iSample] = xTransformed;
            //yTransformedArray[0][iSample] = yTransformed;
            
            const f32 x0 = FLT_FLOOR(xTransformed);
            const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;
            
            const f32 y0 = FLT_FLOOR(yTransformed);
            const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;
            
            // If out of bounds, continue
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              continue;
            }
            
            numInBounds++;
            
            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1.0f - alphaX;
            
            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;
            
            const s32 y0S32 = RoundS32(y0);
            const s32 y1S32 = RoundS32(y1);
            const s32 x0S32 = RoundS32(x0);
            
            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);
            
            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);
            
            const f32 interpolatedPixelF32 = InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse);
            
            const JacobianSample curJacobianSample = pJacobianSamplePyramid[iSample];
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
              //printf("(%f,%f) ", xOriginal, yOriginal);
              
              // This is the only stuff that depends on the current sample
              const f32 templatePixelValue = curSample.grayvalue;
              const f32 xGradientValue = scaleOverFiveTen * curSample.xGradient;
              const f32 yGradientValue = scaleOverFiveTen * curSample.yGradient;
              
              const f32 tGradientValue = oneOverTwoFiftyFive * (interpolatedPixelF32 - templatePixelValue);
              
              const f32 dWu_dtx = curJacobianSample.dWu_dtx;
              //const f32 dWv_dtx = curJacobianSample.dWv_dtx;  // always zero
              //const f32 dWu_dty = curJacobianSample.dWu_dty;  // always zero
              const f32 dWv_dty = curJacobianSample.dWv_dty;
              const f32 dWu_dtz = curJacobianSample.dWu_dtz;
              const f32 dWv_dtz = curJacobianSample.dWv_dtz;
              
              const f32 dWu_dthetaX = curJacobianSample.dWu_dthetaX;
              const f32 dWv_dthetaX = curJacobianSample.dWv_dthetaX;
              const f32 dWu_dthetaY = curJacobianSample.dWu_dthetaY;
              const f32 dWv_dthetaY = curJacobianSample.dWv_dthetaY;
              const f32 dWu_dthetaZ = curJacobianSample.dWu_dthetaZ;
              const f32 dWv_dthetaZ = curJacobianSample.dWv_dthetaZ;
              
              const f32 values[6] = {
                xGradientValue*dWu_dthetaX + yGradientValue*dWv_dthetaX,
                xGradientValue*dWu_dthetaY + yGradientValue*dWv_dthetaY,
                xGradientValue*dWu_dthetaZ + yGradientValue*dWv_dthetaZ,
                xGradientValue*dWu_dtx /*+ yGradientValue*dWv_dtx*/,
                /*xGradientValue*dWu_dty + */yGradientValue*dWv_dty,
                xGradientValue*dWu_dtz + yGradientValue*dWv_dtz
              };
              
              /*
              for(s32 i=0; i<6; ++i) {
                debugA[iSample][i] = values[i];
              }
               */
              
              for(s32 ia=0; ia<6; ia++) {
                for(s32 ja=ia; ja<6; ja++) {
                  AWAt_raw[ia][ja] += values[ia] * values[ja];
                }
                b_raw[ia] += values[ia] * tGradientValue;
              }
            }
          } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)

          
          if(numInBounds < 16) {
            AnkiWarn("LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Projective", "Template drifted too far out of image.");
            return RESULT_OK;
          }
          
          for(s32 ia=0; ia<6; ia++) {
            for(s32 ja=ia; ja<6; ja++) {
              AWAt[ia][ja] = AWAt_raw[ia][ja];
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
          printf("Raw angle update = (%f,%f,%f) degrees, Raw translation udpate = (%f,%f,%f)\n",
                 RAD_TO_DEG(b[0][0]), RAD_TO_DEG(b[0][1]), RAD_TO_DEG(b[0][2]),
                 b[0][3], b[0][4], b[0][5]);
           */
          
          //b.Print("New update");
          
          // Update the 6DoF parameters
          // Note: we are subtracting the update because we're using an _inverse_
          // compositional LK tracking scheme.
          /*
          RAD_TO_DEG(this->params6DoF.angle_x),
                 RAD_TO_DEG(this->params6DoF.angle_y),
                 RAD_TO_DEG(this->params6DoF.angle_z),
                 this->params6DoF.translation.x,
                 this->params6DoF.translation.y,
                 this->params6DoF.translation.z);
            */
          
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
          //const f32 minChange = UpdatePreviousCorners(transformation, previousCorners, scratch);
          
          //if(minChange < convergenceTolerance * scale)
          // TODO: make these parameters
          const f32 angleConvergenceTolerance = scale*DEG_TO_RAD(0.25f);
          const f32 transConvergenceTolerance = scale*0.25f;
          
          if(fabs(b[0][0]) < angleConvergenceTolerance &&
             fabs(b[0][1]) < angleConvergenceTolerance &&
             fabs(b[0][2]) < angleConvergenceTolerance &&
             fabs(b[0][3]) < transConvergenceTolerance &&
             fabs(b[0][4]) < transConvergenceTolerance &&
             fabs(b[0][5]) < transConvergenceTolerance)
          {
          /*
            printf("Final params converged at scale %d: angles = (%f,%f,%f) "
                   "degrees, translation = (%f,%f,%f)\n",
                   whichScale,
                   RAD_TO_DEG(this->params6DoF.angle_x),
                   RAD_TO_DEG(this->params6DoF.angle_y),
                   RAD_TO_DEG(this->params6DoF.angle_z),
                   this->params6DoF.translation.x,
                   this->params6DoF.translation.y,
                   this->params6DoF.translation.z);
           */
            
            verify_converged = true;
            return RESULT_OK;
          }
        } // for(s32 iteration=0; iteration<maxIterations; iteration++)
        
        
        return RESULT_OK;
      } // Result LucasKanadeTracker_SampledPlanar6dof::IterativelyRefineTrack_Projective()
      
      Result LucasKanadeTracker_SampledPlanar6dof::VerifyTrack_Projective(
                                                                          const Array<u8> &nextImage,
                                                                          const u8 verify_maxPixelDifference,
                                                                          s32 &verify_meanAbsoluteDifference,
                                                                          s32 &verify_numInBounds,
                                                                          s32 &verify_numSimilarPixels,
                                                                          MemoryStack scratch) const
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);
        const s32 verify_maxPixelDifferenceS32 = verify_maxPixelDifference;
        
        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);
        
        const s32 whichScale = 0;
        
        const s32 initialImageScaleS32 = BASE_IMAGE_WIDTH / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);
        
        const Point<f32> centerOffsetScaled = this->transformation.get_centerOffset(initialImageScaleF32);
        
        // Initialize with some very extreme coordinates
        FixedLengthList<Quadrilateral<f32> > previousCorners(NUM_PREVIOUS_QUADS_TO_COMPARE, scratch);
        
        for(s32 i=0; i<NUM_PREVIOUS_QUADS_TO_COMPARE; i++) {
          previousCorners[i] = Quadrilateral<f32>(Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f), Point<f32>(-1e10f,-1e10f));
        }
        
        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;
        
        const TemplateSample * restrict pTemplateSamplePyramid = this->templateSamplePyramid[whichScale].Pointer(0);
        
        const s32 numTemplateSamples = this->get_numTemplatePixels(whichScale);
        
        const Array<f32> &homography = this->transformation.get_homography();
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
        const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32;
        const f32 h22 = homography[2][2];
        
        verify_numInBounds = 0;
        verify_numSimilarPixels = 0;
        s32 totalGrayvalueDifference = 0;
        
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
          
          const f32 x0 = FLT_FLOOR(xTransformed);
          const f32 x1 = ceilf(xTransformed); // x0 + 1.0f;
          
          const f32 y0 = FLT_FLOOR(yTransformed);
          const f32 y1 = ceilf(yTransformed); // y0 + 1.0f;
          
          // If out of bounds, continue
          if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
            continue;
          }
          
          verify_numInBounds++;
          
          const f32 alphaX = xTransformed - x0;
          const f32 alphaXinverse = 1 - alphaX;
          
          const f32 alphaY = yTransformed - y0;
          const f32 alphaYinverse = 1.0f - alphaY;
          
          const s32 y0S32 = RoundS32(y0);
          const s32 y1S32 = RoundS32(y1);
          const s32 x0S32 = RoundS32(x0);
          
          const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
          const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);
          
          const f32 pixelTL = *pReference_y0;
          const f32 pixelTR = *(pReference_y0+1);
          const f32 pixelBL = *pReference_y1;
          const f32 pixelBR = *(pReference_y1+1);
          
          const s32 interpolatedPixelValue = RoundS32(InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse));
          const s32 templatePixelValue = RoundS32(curSample.grayvalue);
          const s32 grayvalueDifference = ABS(interpolatedPixelValue - templatePixelValue);
          
          totalGrayvalueDifference += grayvalueDifference;
          
          if(grayvalueDifference <= verify_maxPixelDifferenceS32) {
            verify_numSimilarPixels++;
          }
        } // for(s32 iSample=0; iSample<numTemplateSamples; iSample++)
        
        verify_meanAbsoluteDifference = totalGrayvalueDifference / verify_numInBounds;
        
        return RESULT_OK;
      }
      
      Result LucasKanadeTracker_SampledPlanar6dof::ApproximateSelect(const Array<f32> &magnitudeVector, const s32 numBins, const s32 numToSelect, s32 &numSelected, Array<u16> &magnitudeIndexes)
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
        
        u16 * restrict pMagnitudeIndexes = magnitudeIndexes.Pointer(0,0);
        
        for(s32 i=0; i<numMagnitudes; i++) {
          if(pMagnitudeVector[i] > foundThreshold) {
            pMagnitudeIndexes[numSelected] = static_cast<u16>(i);
            numSelected++;
          }
        }
        
        return RESULT_OK;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
