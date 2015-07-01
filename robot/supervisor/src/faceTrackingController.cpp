/**
 * File: faceTrackingController.cpp
 *
 * Author: Andrew Stein (andrew)
 * Created: 9/29/2014
 *
 * Description:
 *
 *   Controller for "tracking" a face, meaning keeping the robot's attention
 *   focused on the face, by turning its body and tilting its head to keep the
 *   face centered in its field of view. In other words, this is a pan-tilt
 *   controller where the face detections coming from the vision system are used
 *   as the control signal.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "anki/common/robot/errorHandling.h"

#include "anki/cozmo/robot/hal.h"

#include "faceTrackingController.h"
#include "headController.h"
#include "localization.h"
#include "steeringController.h"


namespace Anki {
  namespace Cozmo {
    namespace FaceTrackingController {

      namespace {
        
        bool _isInitialized = false;
        
        s32 _xImageCenter, _yImageCenter;
        
        FaceSelectionMethod _selectionMethod;
        
        f32 _focalLength_pix; // scaled to detection resolution
        
        u32 _timeoutDuration_usec;
        
        bool _isStarted;
        bool _isTracking;
        
        struct TrackedFace {
          // Center of the currently-tracked face in image coordinates
          s32 xCen;
          s32 yCen;
          s32 height;
          
          u32 timeoutTime_usec;
          
          void Set(s32 x, s32 y, s32 h) {
            xCen = x;
            yCen = y;
            height = h;
            
            // Every time we update the tracked face, move the timeout time
            // forward
            timeoutTime_usec = HAL::GetMicroCounter() + _timeoutDuration_usec;
          }
        };
        
        TrackedFace _currentFace, _previousFace;
        
        Messages::FaceDetection faceDetectMsg_;
        bool faceDetectMsgReady_ = false;
      } // "private" namespace
      
      
      Result Init(const int faceDetectionWidth, const int faceDetectionHeight)
      {
        //AnkiConditionalErrorAndReturnValue(params.isInitialized, RESULT_FAIL, "FaceTrackingController.Init.UninitializedParameters", "FaceDetectionParameters should be initialized when passed to FaceTrackingController::Init().\n");
        
        _xImageCenter = faceDetectionWidth/2;
        _yImageCenter = faceDetectionHeight/2;
        
        _selectionMethod = LARGEST;
        
        _isStarted  = false;
        _isTracking = false;
        
        const HAL::CameraInfo* camInfo = HAL::GetHeadCamInfo();
        AnkiConditionalErrorAndReturnValue(camInfo != NULL, RESULT_FAIL,
                                           "FaceTrackingController.Init.NullCamInfo", "Got null pointer from HAL for camera info.\n");
        
        // Get scaled focal length for the resolution we're actually doing detection
        const f32 adjFactor = static_cast<f32>(faceDetectionHeight) / static_cast<f32>(camInfo->nrows);
        _focalLength_pix = camInfo->focalLength_y * adjFactor;
        
        _isInitialized = true;
        
        return RESULT_OK;
      }
      
      
      Result Reset()
      {
        // Clear any remaining face detections left in the mailbox
        faceDetectMsgReady_ = false;
        
        _isStarted = false;
        _isTracking = false;
        
        return RESULT_OK;
      } // Reset()
      
      
      bool IsBusy() {
        return _isStarted;
      }
      
      
      Result StartTracking(FaceSelectionMethod method, u32 timeout_sec)
      {
        Result lastResult = RESULT_OK;
       
        AnkiConditionalErrorAndReturnValue(_isInitialized, RESULT_FAIL,
                                           "FaceTrackingController.StartTracking.NotInitialized",
                                           "FaceTrackingController should be initialized before calling StartTracking().\n");
        lastResult = Reset();
        
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "FaceTrackingController.StartTracking.ResetFailed",
                                           "FaceTrackingController failed calling Reset().\n");
        
        _selectionMethod = method;
        
        _timeoutDuration_usec = timeout_sec * 1e6;
        
        // Disabled when doing vision processing on the basestation
        //lastResult = VisionSystem::StartDetectingFaces();
        
        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK, lastResult,
                                           "FaceTrackingController.StartTracking.StartDetectingFacesFailed",
                                           "FaceTrackingController failed calling VisionSystem::StartDetectingFaces().\n");
        
        _isStarted = true;
        
        return lastResult;
      } // StartTracking()
      
      
      Result Update()
      {
        Result lastResult = RESULT_OK;
        
        AnkiConditionalErrorAndReturnValue(_isInitialized, RESULT_FAIL,
                                           "FaceTrackingController.Update.NotInitialized",
                                           "FaceTrackingController should be initialized before calling Update().\n");
        
        if(_isStarted)
        {
          // Get any docking error signal available from the vision system
          // and update our path accordingly.
          bool gotNewTrack = false;
          _previousFace = _currentFace;
          s32 largestDiagSizeSq = -1;
          s32 smallestCenterDistSq = s32_MAX;
          while( faceDetectMsgReady_ )
          {
            faceDetectMsgReady_ = false;
            if(_isTracking) {
              // If we are currently tracking a face, use the detection closest
              // to the one we've been tracking
              const s32 xCen = faceDetectMsg_.x_upperLeft + faceDetectMsg_.width/2;
              const s32 yCen = faceDetectMsg_.y_upperLeft + faceDetectMsg_.height/2;
              const s32 xDist = xCen - _currentFace.xCen;
              const s32 yDist = yCen - _currentFace.yCen;
              const s32 centerDistSq = xDist*xDist + yDist*yDist;
              if(centerDistSq < smallestCenterDistSq) {
                smallestCenterDistSq = centerDistSq;
                _currentFace.Set(xCen, yCen, faceDetectMsg_.height);
                
                gotNewTrack = true;
              }
              
            } else {
              // If we are not yet tracking, start tracking the one that is largest
              // or closest to center
              switch(_selectionMethod)
              {
                case LARGEST:
                {
                  const s32 diagSizeSq = (faceDetectMsg_.height*faceDetectMsg_.height +
                                          faceDetectMsg_.width*faceDetectMsg_.width);
                  
                  if(diagSizeSq > largestDiagSizeSq) {
                    largestDiagSizeSq = diagSizeSq;
                    
                    _currentFace.Set(faceDetectMsg_.x_upperLeft + faceDetectMsg_.width/2,
                                     faceDetectMsg_.y_upperLeft + faceDetectMsg_.height/2,
                                     faceDetectMsg_.height);
                    
                    gotNewTrack = true;
                  }
                  
                  break;
                } // case LARGEST
                  
                case CENTERED:
                {
                  const s32 xCen = faceDetectMsg_.x_upperLeft + faceDetectMsg_.width/2;
                  const s32 yCen = faceDetectMsg_.y_upperLeft + faceDetectMsg_.height/2;
                  const s32 xDist = xCen - _xImageCenter;
                  const s32 yDist = yCen - _yImageCenter;
                  const s32 centerDistSq = xDist*xDist + yDist*yDist;
                  if(centerDistSq < smallestCenterDistSq) {
                    smallestCenterDistSq = centerDistSq;
                    
                    _currentFace.Set(xCen, yCen, faceDetectMsg_.height);
                    
                    gotNewTrack = true;
                  }
                  
                  break;
                } // case CENTERED
                  
                default:
                  AnkiError("FaceTrackingController.Update.InvalidSelectionMethod",
                            "Unrecognized selectionMethod %d.\n", _selectionMethod);
                  return RESULT_FAIL_INVALID_PARAMETER;
              } // switch(_selectionMethod)
            }
          } // while( Messages::CheckMailbox(faceDetectMsg) )
          
          if(gotNewTrack) {
            _isTracking = true;
            
            // Convert image positions to desired angles
            const f32 yError_pix = static_cast<f32>(_yImageCenter - _currentFace.yCen);
            const f32 tiltAngle = (atan_fast(yError_pix / _focalLength_pix) +
                                   HeadController::GetAngleRad());
            
            const f32 xError_pix = static_cast<f32>(_xImageCenter - _currentFace.xCen);
            const Radians panAngle = (atan_fast(xError_pix / _focalLength_pix) +
                                      Localization::GetCurrentMatOrientation());
            
            PRINT("FaceTrackingController yErr=%.1f: tiltAngle=%.1fdeg, xErr=%.1f: panAngle=%.1fdeg\n",
                  yError_pix, RAD_TO_DEG(tiltAngle), xError_pix, panAngle.getDegrees());

            // Command robot to adjust head and orientation to keep face
            // centered
            HeadController::SetDesiredAngle(tiltAngle);
            SteeringController::ExecuteDirectDrive(-100 * (panAngle - Localization::GetCurrentMatOrientation()).ToFloat(),
                                                    100 * (panAngle - Localization::GetCurrentMatOrientation()).ToFloat());
            /*
            // TODO: Make velocity/acceleration values into parameters
            const f32 turnVelocity = (xError_pix < 0 ? -10.f : 10.f);
            SteeringController::ExecutePointTurn(panAngle.ToFloat(),
                                                 turnVelocity, 5, -5);
            */
          } else if(_isTracking && HAL::GetMicroCounter() > _currentFace.timeoutTime_usec) {
            PRINT("FaceTrackingController timed out. Stopping tracking.\n");
            //VisionSystem::StopTracking();
            _isStarted  = false;
            _isTracking = false;
          }
        } // if(_isStarted)
        
        return lastResult;
      } // Update()

      
      void SetObjectPositionMessage(const Messages::FaceDetection& msg)
      {
        faceDetectMsg_ = msg;
        faceDetectMsgReady_ = true;
      }
      
    } // namespace FaceTrackingController
  } // namespace Cozmo
} // namespace Anki
