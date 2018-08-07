/**
 * File: panTiltTracker.cpp
 *
 * Author: Andrew Stein (andrew)
 * Created: 6/30/2015
 *
 * Description:
 *
 *   Helper class for maintaining a tracked "target" in an image over time and
 *   computing pan and tilt angles for keeping that target centered.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "coretech/vision/engine/cameraCalibration.h"
#include "coretech/vision/engine/panTiltTracker.h"

#include "coretech/common/engine/math/rect_impl.h"

#include "coretech/common/engine/utils/timer.h"
#include "util/logging/logging.h"

namespace Anki {
  namespace Vision {
    
    
    Result PanTiltTracker::Init(const CameraCalibration& camCalib)
    {
      _xImageCenter = camCalib.GetNcols()/2;
      _yImageCenter = camCalib.GetNrows()/2;
      
      _selectionMethod = LARGEST;
      
      _isStarted  = false;
      _isTracking = false;
      
      _focalLength_pix = camCalib.GetFocalLength_x();
      
      _isInitialized = true;
      
      return RESULT_OK;
    }
    
    
    Result PanTiltTracker::Reset()
    {
      _isStarted = false;
      _isTracking = false;
      
      return RESULT_OK;
    } // Reset()
    
    
    bool PanTiltTracker::IsBusy() {
      return _isStarted;
    }
    
    
    Result PanTiltTracker::StartTracking(TargetSelectionMethod method, u32 timeout_ms)
    {
      if(!_isInitialized) {
        PRINT_NAMED_ERROR("PanTiltTracker.StartTracking.NotInitialized",
                          "PanTiltTracker should be initialized before calling StartTracking()");
        return RESULT_FAIL;
      }
      
      Result lastResult = Reset();
      
      if(lastResult != RESULT_OK) {
        PRINT_NAMED_ERROR("PanTiltTracker.StartTracking.ResetFailed",
                          "PanTiltTracker failed calling Reset()");
        return lastResult;
      }
      
      _selectionMethod = method;
      
      _timeoutDuration_ms = timeout_ms * 1e6;
      
      _isStarted = true;
      
      return lastResult;
    } // StartTracking()
    
    
    void PanTiltTracker::Target::Set(s32 x, s32 y, s32 h)
    {
      xCen = x;
      yCen = y;
      height = h;
      // Every time we update the tracked face, move the timeout time forward
      // pterry 2017/02/16:  This used to be +=; I've 'fixed' but this code is not used
      timeoutTime_ms = BaseStationTimer::getInstance()->GetCurrentTimeStamp();
    }
    
    Result PanTiltTracker::Update(std::vector<Rectangle<s32> >& targets,
                                  const TimeStamp_t observationTime_ms,
                                  Radians& newPanAngle, Radians& newTiltAngle)
    {
      Result lastResult = RESULT_OK;
      
      // If anything goes wrong or we aren't started, default fallback behavior
      // is to say "do nothing"
      newPanAngle = 0;
      newTiltAngle = 0;
      
      if(!_isInitialized) {
        PRINT_NAMED_ERROR("PanTiltTracker.Update.NotInitialized",
                          "PanTiltTracker should be initialized before calling Update()");
        return RESULT_FAIL;
      }
      
      if(_isStarted)
      {
        bool gotNewTrack = false;
        _previousTarget = _currentTarget;
        s32 largestDiagSizeSq = -1;
        s32 smallestCenterDistSq = std::numeric_limits<s32>::max();
        for(auto & target : targets)
        {
          if(_isTracking) {
            // If we are currently tracking a face, use the detection closest
            // to the one we've been tracking
            const s32 xCen = target.GetXmid();
            const s32 yCen = target.GetYmid();
            const s32 xDist = xCen - _currentTarget.xCen;
            const s32 yDist = yCen - _currentTarget.yCen;
            const s32 centerDistSq = xDist*xDist + yDist*yDist;
            if(centerDistSq < smallestCenterDistSq) {
              smallestCenterDistSq = centerDistSq;
              _currentTarget.Set(xCen, yCen, target.GetHeight());
              
              gotNewTrack = true;
            }
            
          } else {
            // If we are not yet tracking, start tracking the one that is largest
            // or closest to center
            switch(_selectionMethod)
            {
              case LARGEST:
              {
                const s32 diagSizeSq = (target.GetHeight()*target.GetHeight() +
                                        target.GetWidth()*target.GetWidth());
                
                if(diagSizeSq > largestDiagSizeSq) {
                  largestDiagSizeSq = diagSizeSq;
                  
                  _currentTarget.Set(target.GetXmid(), target.GetYmid(), target.GetHeight());
                  
                  gotNewTrack = true;
                }
                
                break;
              } // case LARGEST
                
              case CENTERED:
              {
                const s32 xCen = target.GetXmid();
                const s32 yCen = target.GetYmid();
                const s32 xDist = xCen - _xImageCenter;
                const s32 yDist = yCen - _yImageCenter;
                const s32 centerDistSq = xDist*xDist + yDist*yDist;
                if(centerDistSq < smallestCenterDistSq) {
                  smallestCenterDistSq = centerDistSq;
                  
                  _currentTarget.Set(xCen, yCen, target.GetHeight());
                  
                  gotNewTrack = true;
                }
                
                break;
              } // case CENTERED
                
              default:
                PRINT_NAMED_ERROR("PanTiltTracker.Update.InvalidSelectionMethod",
                                  "Unrecognized selectionMethod %d", _selectionMethod);
                return RESULT_FAIL_INVALID_PARAMETER;
            } // switch(_selectionMethod)
          }
        } // while( Messages::CheckMailbox(faceDetectMsg) )
        
        if(gotNewTrack) {
          _isTracking = true;
          
          // Convert image positions to desired angles
          const f32 yError_pix = static_cast<f32>(_yImageCenter - _currentTarget.yCen);
          newTiltAngle = std::atan(yError_pix / _focalLength_pix);
          
          const f32 xError_pix = static_cast<f32>(_xImageCenter - _currentTarget.xCen);
          newPanAngle = std::atan(xError_pix / _focalLength_pix);
          
          PRINT_NAMED_INFO("PanTiltTracker.Update.NewTrack",
                           "yErr=%.1f: tiltAngle=%.1fdeg, xErr=%.1f: panAngle=%.1fdeg",
                           yError_pix, newTiltAngle.getDegrees(),
                           xError_pix, newPanAngle.getDegrees());
          
        } else if(_isTracking && BaseStationTimer::getInstance()->GetCurrentTimeStamp() > _currentTarget.timeoutTime_ms)
        {
          PRINT_NAMED_INFO("PanTiltTracker.Update.TimedOut",  "Timed out. Stopping tracking.");

          _isStarted  = false;
          _isTracking = false;
        }
      } // if(_isStarted)
      
      return lastResult;
    } // Update()
    
} // namespace Vision
} // namespace Anki
