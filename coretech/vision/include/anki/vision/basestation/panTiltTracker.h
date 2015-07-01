/**
 * File: panTiltTracker.h
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

#ifndef ANKI_CORETECH_VISION_PANTILTTRACKER_H
#define ANKI_CORETECH_VISION_PANTILTTRACKER_H

#include "anki/common/types.h"

#include "anki/common/basestation/math/rect.h"

#include "anki/common/shared/radians.h"


namespace Anki {
  
  namespace Vision {
    
    // Forward declaration
    class CameraCalibration;
    
    class PanTiltTracker
    {
    public:
      enum TargetSelectionMethod {
        LARGEST,
        CENTERED
      };
      
      Result Init(const CameraCalibration& camCalib);
      
      Result StartTracking(TargetSelectionMethod method, u32 timeout_ms);
      
      Result Update(std::vector<Rectangle<s32> >& targets,
                    const TimeStamp_t observationTime_ms,
                    Radians& newPanAngle, Radians& newTiltAngle);
      
      Result Reset();
      
      // Returns true if tracker is currently looking for targets, whether or not
      // it is actively tracking. If tracking times out, this will return false.
      bool IsBusy();
      
    private:
      bool _isInitialized = false;
      
      s32 _xImageCenter, _yImageCenter;
      
      TargetSelectionMethod _selectionMethod;
      
      f32 _focalLength_pix; // scaled to detection resolution
      
      u32 _timeoutDuration_ms;
      
      bool _isStarted;
      bool _isTracking;
      
      struct Target {
        // Center of the currently-tracked face in image coordinates
        s32 xCen;
        s32 yCen;
        s32 height;
        
        u32 timeoutTime_ms;
        
        void Set(s32 x, s32 y, s32 h);
      };
      
      Target _currentTarget, _previousTarget;
      
    }; // class PanTiltTracker
    
  } // namespace Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_PANTILTTRACKER_H
