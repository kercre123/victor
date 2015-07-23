/**
 * File: matlabVisionProcessor.cpp
 *
 * Author: Andrew Stein
 * Date:   3/28/2014
 *
 * Description: For testing vision algorithms implemented in Matlab.
 *
 * Copyright: Anki, Inc. 2014
 **/


#if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR

#include "anki/common/types.h"
#include "anki/common/robot/matlabInterface.h"

#include "anki/cozmo/robot/hal.h"

#include "matlabVisionProcessor.h"
#include "matlabVisualization.h"
#include "visionParameters.h"

#ifndef DOCKING_ALGORITHM
#error DOCKING_ALGORITHM should be defined to use matlabVisionProcessor.cpp!
#endif

namespace Anki {
  namespace Cozmo {
    namespace MatlabVisionProcessor {
      
      static Matlab matlabProc_(false);
      static bool isInitialized_ = false;
      static bool haveTemplate_;
      static bool initTrackerAtFullRes_;
      
      static VisionSystem::TrackerParameters trackerParameters_;
      static VisionSystem::DetectFiducialMarkersParameters detectionParameters_;
      
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
      static Transformations::TransformType transformType_ = Transformations::TRANSFORM_AFFINE;
      const char* transformTypeStr_ = "affine";
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE || DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
      static Transformations::TransformType transformType_ = Transformations::TRANSFORM_PROJECTIVE;
      const char* transformTypeStr_ = "homography";
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
      static Transformations::TransformType transformType_ = Transformations::TRANSFORM_PROJECTIVE;
      const char* transformTypeStr_ = "planar6dof";
#else
#error Can't use Matlab tracker with anything other than affine, projective, or planar6dof.
#endif
      
      // Now coming from trackerParameters_
      //static HAL::CameraMode trackingResolution_;
      //static f32 scaleTemplateRegionPercent_;
      //static s32 maxIterations_;
      //static f32 convergenceTolerance_;
      
      static f32 errorTolerance_;
      static s32 scaleFactor_;
      
      Result Initialize()
      {
        if(!isInitialized_) {
          matlabProc_.EvalStringEcho("run(fullfile('..','..','..','..','matlab','initCozmoPath')); "
                                     "camera = Camera('calibration', struct("
                                     "   'fc', [%f %f], 'cc', [%f %f], "
                                     "   'kc', zeros(5,1), 'alpha_c', 0)); ",
                                     VisionSystem::GetCameraCalibration()->focalLength_x,
                                     VisionSystem::GetCameraCalibration()->focalLength_x,
                                     VisionSystem::GetCameraCalibration()->center_x,
                                     VisionSystem::GetCameraCalibration()->center_y);
          
          trackerParameters_.Initialize();
          detectionParameters_.Initialize();
          
          initTrackerAtFullRes_ = false;
          
          //scaleFactor_ = (1<<VisionSystem::CameraModeInfo[Vision::CAMERA_RES_QVGA].downsamplePower[trackerParameters_.trackingResolution]);
          scaleFactor_ = (Vision::CameraResInfo[Vision::CAMERA_RES_QVGA].width /
                          Vision::CameraResInfo[trackerParameters_.trackingResolution].width);
          haveTemplate_ = false;
          
          //scaleTemplateRegionPercent_ = 0.1f;
          //maxIterations_ = 25;
          //convergenceTolerance_ = 1.f;
          
          errorTolerance_ = 0.5f;
          
          isInitialized_ = true;
        }
        
        return RESULT_OK;
      } // MatlabVisionProcess::Initialize()
      
      
      
      Result DetectMarkers(const Array<u8>& img,
                           FixedLengthList<VisionMarker>& markers,
                           FixedLengthList<Array<f32> >& homographies,
                           MemoryStack scratch)
      {
        if(!isInitialized_) {
          return RESULT_FAIL;
        }
        
        matlabProc_.PutArray(img, "img");
        
        // TODO: Pass through more parameters to the matlab detector
        matlabProc_.EvalStringEcho("markers = simpleDetector(img, "
                                   "  'quadRefinementIterations', %d); "
                                   "numMarkers = length(markers); ",
                                   detectionParameters_.quadRefinementIterations);
        
        const s32 numMarkers = static_cast<s32>(mxGetScalar(matlabProc_.GetArray("numMarkers")));
        
        AnkiConditionalErrorAndReturnValue(numMarkers < markers.get_maximumSize(),
                                           RESULT_FAIL_INVALID_SIZE,
                                           "MatlabVisionProcessor::DetectMarkers",
                                           "More markers detected than there was room for in output markers list.");
        
        AnkiConditionalErrorAndReturnValue(numMarkers < homographies.get_maximumSize(),
                                           RESULT_FAIL_INVALID_SIZE,
                                           "MatlabVisionProcessor::DetectMarkers",
                                           "More markers detected than there was room for in homographies list.");
        
        for(s32 i=0; i<numMarkers; ++i) {
          VisionMarker& currentMarker = markers[i];
          
          matlabProc_.EvalStringEcho("marker     = markers{%d}; "
                                     "corners    = marker.corners - 1; "
                                     "markerType = uint32(marker.codeID) - 1; "
                                     "H          = single(marker.H); ",
                                     i+1);
          
          currentMarker.corners    = matlabProc_.GetQuad<f32>("corners");
          
          {
            // Matlab produces an oriented marker type (code). We will use that
            // to get the unoriented code using the LUT here:
            using namespace VisionMarkerDecisionTree;
            OrientedMarkerLabel orientedMarkerType = static_cast<OrientedMarkerLabel>(mxGetScalar(matlabProc_.GetArray("markerType")));
            currentMarker.markerType = RemoveOrientationLUT[orientedMarkerType];
          }

          currentMarker.isValid    = true;
          
          homographies[i].Set(matlabProc_.GetArray<f32>("H", scratch));
          
          // TODO: Set observedOrientation? or is it used anywhere?
          // currentMarker.observedOrientation = ?
          
        } // for each marker
        
        markers.set_size(numMarkers);
        homographies.set_size(numMarkers);
        
        return RESULT_OK;
      
      } // DetectMarkers()
      
      
      Result InitTemplate(const Array<u8>& imgFull,
                              const Quadrilateral<f32>& trackingQuad,
                              MemoryStack scratch)
      {
        if(!isInitialized_) {
          return RESULT_FAIL;
        }
        
        matlabProc_.PutQuad(trackingQuad, "initTrackingQuad");
        
        if(initTrackerAtFullRes_ || (imgFull.get_size(0) == trackerParameters_.trackingImageHeight &&
                                     imgFull.get_size(1) == trackerParameters_.trackingImageWidth))
        {
          matlabProc_.PutArray(imgFull, "img");
        }
        else {
          Array<u8> imgSmall(trackerParameters_.trackingImageHeight,
                             trackerParameters_.trackingImageWidth,
                             scratch);
          
          VisionSystem::DownsampleHelper(imgFull, imgSmall, scratch);
          matlabProc_.PutArray(imgSmall, "img");
          matlabProc_.EvalStringEcho("initTrackingQuad = initTrackingQuad / %d;",
                                     scaleFactor_);
        }
        
        matlabProc_.EvalStringEcho("marker3d = (%f/2)*[-1 0 1; -1 0 -1; 1 0 1; 1 0 -1];",
                                   VisionSystem::GetTrackingMarkerWidth());
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
        matlabProc_.EvalStringEcho("LKtracker = LucasKanadeTracker(img, "
                                   "  initTrackingQuad + 1, "
                                   "  'Type', '%s', 'RidgeWeight', 0, "
                                   "  'DebugDisplay', false, 'UseBlurring', false, "
                                   "  'UseNormalization', false, "
                                   "  'TrackingResolution', [%d %d], "
                                   "  'TemplateRegionPaddingFraction', %f, "
                                   "  'NumScales', %d, "
                                   "  'NumSamples', %d, "
                                   "  'Num"
                                   "  'CalibrationMatrix', [%f 0 %f; 0 %f %f; 0 0 1] / %d, "
                                   "  'MarkerWidth', %f);",
                                   transformTypeStr_,
                                   trackerParameters_.trackingImageWidth,
                                   trackerParameters_.trackingImageHeight,
                                   trackerParameters_.scaleTemplateRegionPercent - 1.f,
                                   trackerParameters_.numPyramidLevels,
                                   trackerParameters_.numInteriorSamples,
                                   VisionSystem::GetCameraCalibration()->focalLength_x,
                                   VisionSystem::GetCameraCalibration()->center_x,
                                   VisionSystem::GetCameraCalibration()->focalLength_y,
                                   VisionSystem::GetCameraCalibration()->center_y,
                                   scaleFactor_,
                                   VisionSystem::GetTrackingMarkerWidth());
#else
        matlabProc_.EvalStringEcho("LKtracker = LucasKanadeTracker(img, "
                                   "  initTrackingQuad + 1, "
                                   "  'Type', '%s', 'RidgeWeight', 0, "
                                   "  'DebugDisplay', false, 'UseBlurring', false, "
                                   "  'UseNormalization', false, "
                                   "  'TrackingResolution', [%d %d], "
                                   "  'TemplateRegionPaddingFraction', %f, "
                                   "  'NumScales', %d, "
                                   "  'NumSamples', %d);",
                                   transformTypeStr_,
                                   trackerParameters_.trackingImageWidth,
                                   trackerParameters_.trackingImageHeight,
                                   trackerParameters_.scaleTemplateRegionPercent - 1.f,
                                   trackerParameters_.numPyramidLevels,
                                   trackerParameters_.maxSamplesAtBaseLevel);
#endif
        
        haveTemplate_ = true;
        
        MatlabVisualization::SendTrackInit(imgFull, trackingQuad);
        
        return RESULT_OK;
      } // MatlabVisionProcessor::InitTemplate()


      void UpdateTracker(const f32 T_fwd_robot, const f32 T_hor_robot,
                         const Radians& theta_robot,
                         const Radians& theta_head)
      {
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
        matlabProc_.EvalStringEcho("LKtracker.UpdatePoseFromRobotMotion(%f, %f, %f, %f);",
                                   T_fwd_robot, T_hor_robot, theta_robot.ToFloat(), theta_head.ToFloat());
#else
      // This call is only valid for for planar6dof tracker
      AnkiAssert(false);
#endif // DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
      }
      
      void UpdateTracker(const Array<f32>& predictionUpdate)
      {
        AnkiAssert(predictionUpdate.get_size(0)==1);
        
        switch(predictionUpdate.get_size(1))
        {
          case 2:
            AnkiAssert(transformType_ == Transformations::TRANSFORM_TRANSLATION);
            
            matlabProc_.EvalStringEcho("update = ([1 0 %f; "
                                       "           0 1 %f; "
                                       "           0 0  1]);",
                                       predictionUpdate[0][0], predictionUpdate[0][1]);
            break;
            
          case 6:
            AnkiAssert(transformType_ == Transformations::TRANSFORM_PROJECTIVE ||
                       transformType_ == Transformations::TRANSFORM_AFFINE);
            
            matlabProc_.EvalStringEcho("update = ([1+%f  %f   %f; "
                                       "            %f  1+%f  %f; "
                                       "             0    0    1]);",
                                       predictionUpdate[0][0], predictionUpdate[0][1],
                                       predictionUpdate[0][2], predictionUpdate[0][3],
                                       predictionUpdate[0][4], predictionUpdate[0][5]);
            break;
            
          default:
            AnkiError("MatlabVisionProcess::UpdateTracker",
                      "Unrecognized tracker transformation update size (%d vs. 2 or 6)",
                      predictionUpdate.get_size(1));
        } // switch
        
        matlabProc_.EvalStringEcho("S = [%d 0 0; 0 %d 0; 0 0 1]; "
                                   "transform = S*double(LKtracker.tform)*inv(S); "
                                   "newTform = transform / update; " // transform * inv(update)
                                   "newTform = inv(S) * newTform * S; "
                                   "newTform = newTform / newTform(3,3); "
                                   "LKtracker.set_tform(newTform);",
                                   scaleFactor_, scaleFactor_);
      } // MatlabVisionProcess::UpdateTracker()
      
      Result TrackTemplate(const Array<u8>& imgFull, bool& converged, MemoryStack scratch)
      {
        if(!haveTemplate_) {
          AnkiWarn("MatlabVisionProcess::TrackTemplate",
                   "TrackTemplate called before tracker initialized.");
          return RESULT_FAIL;
        }
        
        if(imgFull.get_size(0) == trackerParameters_.trackingImageHeight &&
           imgFull.get_size(1) == trackerParameters_.trackingImageWidth)
        {
          matlabProc_.PutArray(imgFull, "img");
        }
        else {
          Array<u8> img(trackerParameters_.trackingImageHeight,
                        trackerParameters_.trackingImageWidth,
                        scratch);
          
          VisionSystem::DownsampleHelper(imgFull, img, scratch);
          matlabProc_.PutArray(img, "img");
        }
        
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
        matlabProc_.EvalStringEcho(//"desktop, keyboard; "
                                   "[converged, reason] = LKtracker.track(img, "
                                   "   'MaxIterations', %d, "
                                   "   'ConvergenceTolerance', struct('angle', 0.1*pi/180, 'distance', 0.1), "
                                   "   'ErrorTolerance', %f); "
                                   //"if ~converged, desktop, keyboard, end, "
                                   "corners = %d*(double(LKtracker.corners) - 1);",
                                   500, //trackerParameters_.maxIterations,
                                   //0.01f, //trackerParameters_.convergenceTolerance,
                                   10.f, //errorTolerance_,
                                   scaleFactor_);
#else
        matlabProc_.EvalStringEcho(//"desktop, keyboard; "
                                   "converged = LKtracker.track(img, "
                                   "   'MaxIterations', %d, "
                                   "   'ConvergenceTolerance', %f, "
                                   "   'ErrorTolerance', %f); "
                                   "corners = %d*(double(LKtracker.corners) - 1);",
                                   trackerParameters_.maxIterations,
                                   trackerParameters_.convergenceTolerance,
                                   errorTolerance_,
                                   scaleFactor_);
#endif
        
        converged = mxIsLogicalScalarTrue(matlabProc_.GetArray("converged"));
        
        //char buffer[100];
        //mxGetString(matlabProc_.GetArray("reason"), buffer, 100);
        //PRINT("%s\n", buffer);
        
        Quadrilateral<f32> quad = matlabProc_.GetQuad<f32>("corners");
        
        MatlabVisualization::SendTrack(imgFull, quad, converged);
        
        return RESULT_OK;
      } // MatlabVisionProcessor::TrackTemplate()
      
      
      Quadrilateral<f32> GetTrackerQuad()
      {
        matlabProc_.EvalStringEcho("currentQuad = %d*(double(LKtracker.corners)-1);", scaleFactor_);
        return matlabProc_.GetQuad<f32>("currentQuad");
      }
      
      
      Transformations::PlanarTransformation_f32 GetTrackerTransform(MemoryStack& memory)
      {
        Array<f32> homography = Array<f32>(3,3,memory);
        
        matlabProc_.EvalStringEcho("S = [%d 0 0; 0 %d 0; 0 0 1]; "
                                   "transform = S*double(LKtracker.tform)*inv(S); "
                                   "transform = transform / transform(3,3); "
                                   "initCorners = %d*(double(LKtracker.initCorners) - 1); "
                                   "xcen = %d*(double(LKtracker.xcen) - 1); "
                                   "ycen = %d*(double(LKtracker.ycen) - 1);",
                                   scaleFactor_, scaleFactor_, scaleFactor_, scaleFactor_, scaleFactor_);
        
        Quadrilateral<f32> quad = matlabProc_.GetQuad<f32>("initCorners");
        const mxArray* mxTform = matlabProc_.GetArray("transform");
        
        AnkiAssert(mxTform != NULL && mxGetM(mxTform) == 3 && mxGetN(mxTform)==3);
        
        const double* mxTformData = mxGetPr(mxTform);
        
        homography[0][0] = mxTformData[0];
        homography[1][0] = mxTformData[1];
        homography[2][0] = mxTformData[2];
        
        homography[0][1] = mxTformData[3];
        homography[1][1] = mxTformData[4];
        homography[2][1] = mxTformData[5];
        
        homography[0][2] = mxTformData[6];
        homography[1][2] = mxTformData[7];
        homography[2][2] = mxTformData[8];
        
        const Point2f centerOffset(mxGetScalar(matlabProc_.GetArray("xcen")),
                                   mxGetScalar(matlabProc_.GetArray("ycen")) );
        
        return Transformations::PlanarTransformation_f32(transformType_, quad, homography, centerOffset, memory);
      } // MatlabVisionProcessor::GetTrackerTransform()
      
      void ComputeProjectiveDockingSignal(const Quadrilateral<f32>& transformedQuad,
                                          f32& x_distErr, f32& y_horErr, f32& z_height,
                                          f32& angleErr)
      {
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
        
        AnkiAssert(false); // Cannot compute projective error signal from affine tracker.
        
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PLANAR6DOF
        // Nothing to compute.  The tracker's parameters are the answer directly
        
        MatlabVisionProcessor::matlabProc_.EvalStringEcho("angleErr = LKtracker.theta_y; "
                                                          "Tz  = LKtracker.tz; "
                                                          "Tx  = LKtracker.tx; "
                                                          "Ty  = LKtracker.ty; ");
#else
        // Compute the pose of the block according to the current homography in
        // the tracker.
        matlabProc_.PutQuad(transformedQuad, "transformedQuad");
        
        matlabProc_.EvalStringEcho("blockPose = camera.computeExtrinsics(transformedQuad, marker3d); "
                                   "angleErr = asin(blockPose.Rmat(3,1)); "
                                   "Tz  = blockPose.T(3); "
                                   "Tx  = blockPose.T(1); "
                                   "Ty  = blockPose.T(2); ");
        
#endif
        
        x_distErr = static_cast<f32>(mxGetScalar(matlabProc_.GetArray("Tx")));
        y_horErr  = static_cast<f32>(mxGetScalar(matlabProc_.GetArray("Ty")));
        z_height  = static_cast<f32>(mxGetScalar(matlabProc_.GetArray("Tz")));
        angleErr  = static_cast<f32>(mxGetScalar(matlabProc_.GetArray("angleErr")));
        
      } // ComputeProjectiveDockingSignal()
      
    } // namespace MatlabVisionProcessor
  } // namespace Cozmo
} // namespace Anki

#endif // if USE_MATLAB_TRACKER || USE_MATLAB_DETECTOR
