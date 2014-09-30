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

#include "faceTrackingController.h"
#include "headController.h"
#include "localization.h"
#include "messages.h"
#include "steeringController.h"
#include "visionParameters.h"

namespace Anki {
  namespace Cozmo {
    namespace FaceTrackingController {

      namespace {
        
        bool _isInitialized = false;
        
        s32 _xImageCenter, _yImageCenter;
        
        FaceSelectionMethod _selectionMethod;
        
        f32 _distanceFactor;
        
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
        
      } // "private" namespace
      
      
      Result Init(const VisionSystem::FaceDetectionParameters& params)
      {
        AnkiConditionalErrorAndReturnValue(params.isInitialized, RESULT_FAIL, "FaceTrackingController.Init.UninitializedParameters", "FaceDetectionParameters should be initialized when passed to FaceTrackingController::Init().\n");
        
        _xImageCenter = params.faceDetectionWidth/2;
        _yImageCenter = params.faceDetectionHeight/2;
        
        _selectionMethod = LARGEST;
        
        _isStarted  = false;
        _isTracking = false;
        
        const HAL::CameraInfo* camInfo = HAL::GetHeadCamInfo();
        AnkiConditionalErrorAndReturnValue(camInfo != nullptr, RESULT_FAIL,
                                           "FaceTrackingController.Init.NullCamInfo", "Got null pointer from HAL for camera info.\n");
        
        // Get scaled focal length for the resolution we're actually doing detection
        const f32 adjFactor = static_cast<f32>(params.faceDetectionHeight) / static_cast<f32>(camInfo->nrows);
        const f32 f_pix = camInfo->focalLength_y * adjFactor;
        
        // Compute factor for converting from detected height in pixels to
        // approximate distance in mm
        //
        //    d_mm = (f_pix * h_mm) / h_pix;
        //
        // i.e., distanceFactor is the numerator of this equation.
        _distanceFactor = static_cast<f32>(params.avgHeadHeight_mm)*f_pix;
        
        _isInitialized = true;
        
        return RESULT_OK;
      }
      
      
      Result Reset()
      {
        // Clear any remaining face detections left in the mailbox
        Messages::FaceDetection faceDetectMsg;
        while( Messages::CheckMailbox(faceDetectMsg) );
        
        _isStarted = false;
        _isTracking = false;
        
        return RESULT_OK;
      } // Reset()
      
      
      bool IsBusy() {
        return _isStarted;
      }
      
      
      Result StartTracking(FaceSelectionMethod method, TimeStamp_t timeout_usec)
      {
        Result lastResult = RESULT_OK;
       
        AnkiConditionalErrorAndReturnValue(_isInitialized, RESULT_FAIL,
                                           "FaceTrackingController.StartTracking.NotInitialized",
                                           "FaceTrackingController should be initialized before calling StartTracking().\n");
        
        _selectionMethod = method;
        
        _timeoutDuration_usec = timeout_usec;
        
        _isStarted = true;
        _isTracking = false;
        
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
          Messages::FaceDetection faceDetectMsg;
          bool gotNewTrack = false;
          _previousFace = _currentFace;
          s32 largestDiagSizeSq = -1;
          s32 smallestCenterDistSq = s32_MAX;
          while( Messages::CheckMailbox(faceDetectMsg) )
          {
            if(_isTracking) {
              // If we are currently tracking a face, use the detection closest
              // to the one we've been tracking
              const s32 xCen = faceDetectMsg.x_upperLeft + faceDetectMsg.width/2;
              const s32 yCen = faceDetectMsg.y_upperLeft + faceDetectMsg.height/2;
              const s32 xDist = xCen - _currentFace.xCen;
              const s32 yDist = yCen - _currentFace.yCen;
              const s32 centerDistSq = xDist*xDist + yDist*yDist;
              if(centerDistSq < smallestCenterDistSq) {
                smallestCenterDistSq = centerDistSq;
                _currentFace.Set(xCen, yCen, faceDetectMsg.height);
                
                gotNewTrack = true;
              }
              
            } else {
              // If we are not yet tracking, start tracking the one that is largest
              // or closest to center
              switch(_selectionMethod)
              {
                case LARGEST:
                {
                  const s32 diagSizeSq = (faceDetectMsg.height*faceDetectMsg.height +
                                          faceDetectMsg.width*faceDetectMsg.width);
                  
                  if(diagSizeSq > largestDiagSizeSq) {
                    largestDiagSizeSq = diagSizeSq;
                    
                    _currentFace.Set(faceDetectMsg.x_upperLeft + faceDetectMsg.width/2,
                                     faceDetectMsg.y_upperLeft + faceDetectMsg.height/2,
                                     faceDetectMsg.height);
                    
                    gotNewTrack = true;
                  }
                  
                  break;
                } // case LARGEST
                  
                case CENTERED:
                {
                  const s32 xCen = faceDetectMsg.x_upperLeft + faceDetectMsg.width/2;
                  const s32 yCen = faceDetectMsg.y_upperLeft + faceDetectMsg.height/2;
                  const s32 xDist = xCen - _xImageCenter;
                  const s32 yDist = yCen - _yImageCenter;
                  const s32 centerDistSq = xDist*xDist + yDist*yDist;
                  if(centerDistSq < smallestCenterDistSq) {
                    smallestCenterDistSq = centerDistSq;
                    
                    _currentFace.Set(xCen, yCen, faceDetectMsg.height);
                    
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
            
            const f32 estimatedDistance_pix = _distanceFactor / static_cast<f32>(_currentFace.height);
            
            const f32 yError_pix = static_cast<f32>(_currentFace.yCen - _yImageCenter);
            const f32 tiltAngle = atan_fast(yError_pix/estimatedDistance_pix);
            
            HeadController::SetDesiredAngle(tiltAngle);
            
            const f32 xError_pix = static_cast<f32>(_currentFace.xCen - _xImageCenter);
            const Radians panAngle = atan_fast(xError_pix/estimatedDistance_pix);
            const Radians targetAngle = Localization::GetCurrentMatOrientation() + panAngle;
            
            // TODO: Make velocity/acceleration values into parameters
            SteeringController::ExecutePointTurn(targetAngle.ToFloat(),
                                                 .05, .01, .01);
            
          } else if(HAL::GetMicroCounter() > _currentFace.timeoutTime_usec) {
            PRINT("FaceTrackingController timed out. Stopping tracking.");
            _isStarted  = false;
            _isTracking = false;
          }
        } // if(_isStarted)
        
        return lastResult;
      } // Update()

    } // namespace FaceTrackingController
  } // namespace Cozmo
} // namespace Anki