/**
 * File: animation.cpp
 *
 * Author: Andrew Stein
 * Created: 10/16/2014
 *
 * Description:
 *
 *   Implements an animation, which consists of a list of keyframes for each
 *   animate-able subsystem, to be played concurrently. Note that a subsystem
 *   can have no keyframes (numFrames==0) and it will simply be ignored (and
 *   left unlocked) during that animation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "animation.h"

#include "headController.h"
#include "liftController.h"

#include "anki/common/robot/utilities.h"

namespace Anki {
namespace Cozmo {
  
  Animation::Animation()
  : _ID(0)
  , _startTime_ms(0)
  , _isPlaying(false)
  , _allSubSystemsReady(false)
  {

  }
  
  
  Animation::KeyFrameList::KeyFrameList()
  : isReady(false)
  , numFrames(0)
  , currFrame(0)
  {
    
  }
  
  
  void Animation::Init()
  {
    PRINT("Init Animation %d\n", _ID);
    
    _startTime_ms = HAL::GetTimeStamp();
    
    _allSubSystemsReady = false;
    
    _isPlaying = true;
    
    /*
    _returnToOrigState = returnToOrigStateWhenDone;
    
    if(_returnToOrigState) {
      _origHeadAngle  = HeadController::GetAngleRad();
      _origLiftHeight = LiftController::GetHeightMM();
      
      // TODO: Need HAL::GetLED() method to store original LED state
     
       for(s32 iLED=0; iLED < HAL::NUM_LEDS; ++iLED) {
       origLEDcolors[iLED] = HAL::
       }
     
    }
     */
    
    // Reset each subsystem and call TransitionInto for any initial keyframes
    // that have a relative start time of 0
    for(s32 iSubSystem=0; iSubSystem<NUM_SUBSYSTEMS; ++iSubSystem) {
      _subSystems[iSubSystem].currFrame = 0;
      _subSystems[iSubSystem].isReady   = false;
      
      if(_subSystems[iSubSystem].numFrames > 0 &&
         _subSystems[iSubSystem].frames[0].relTime_ms == 0)
      {
        _subSystems[iSubSystem].frames[0].TransitionInto(_startTime_ms);
      }
    }
    
  } // Init()
  
  
  bool Animation::CheckSubSystemReadiness()
  {
    if(!_allSubSystemsReady)
    {
      // Mark as true for now, and let the loop below unset it
      _allSubSystemsReady = true;
      
      for(s32 iSubSystem=0; iSubSystem<NUM_SUBSYSTEMS; ++iSubSystem) {
        
        Animation::KeyFrameList& subSystem = _subSystems[iSubSystem];
        
        // If this subsystem isn't marked as ready yet, check it now
        if(!subSystem.isReady) {
          if(subSystem.numFrames == 0) {
            // Subsystem with no keyframes is always ready
            subSystem.isReady = true;
          } else if(subSystem.frames[0].relTime_ms > 0) {
            // Subsystems with first keyframe at time > 0 are always ready
            subSystem.isReady = true;
          } else {
            // Check whether the first keyframe is "in position"
            subSystem.isReady = subSystem.frames[0].IsInPosition();
          }
          
          // In case this subSystem just became ready:
          _allSubSystemsReady &= subSystem.isReady;
          
        } // if(!subSystem.isReady)
        
      } // for each subSystem
      
      if(_allSubSystemsReady) {
        // If all subsystems just became ready, update the start time for this
        // animation to now and call TransitionInto for keyframes that are
        // first for their subsystem but do _not_ have relTime==0
        _startTime_ms = HAL::GetTimeStamp();
        
        for(s32 iSubSystem=0; iSubSystem<NUM_SUBSYSTEMS; ++iSubSystem)
        {
          if(_subSystems[iSubSystem].numFrames > 0 &&
             _subSystems[iSubSystem].frames[0].relTime_ms > 0)
          {
            _subSystems[iSubSystem].frames[0].TransitionInto(_startTime_ms);
          }
        } // for each subSystem
        
      } // if(_allSubSystemsReady)
      
    } // if (!_allSubSystemsReady)
    
    return _allSubSystemsReady;
      
  } // CheckSubSystemReadiness()
  
  
  void Animation::Update()
  {
    if(CheckSubSystemReadiness() == false) {
      // Wait for all subsystems to be ready
      //PRINT("Waiting for all subsystems to be ready to start animation...\n");
      return;
    }

    bool subSystemPlaying[NUM_SUBSYSTEMS];
    
    for(s32 iSubSystem=0; iSubSystem<NUM_SUBSYSTEMS; ++iSubSystem)
    {
      // Get a reference to the current subSystem, for convenience
      Animation::KeyFrameList& subSystem = _subSystems[iSubSystem];

      if(subSystem.numFrames == 0 || subSystem.currFrame >= subSystem.numFrames) {
        // Skip empty or already-finished subsystems
        subSystemPlaying[iSubSystem] = false;
        
      } else {
        // TODO: "Lock" controller for this subsystem
        
        subSystemPlaying[iSubSystem] = true;
        
        // Get a reference to the current frame of the current subsystem, for convenience
        KeyFrame& currKeyFrame = subSystem.frames[subSystem.currFrame];
        
        // Has the current keyframe's time now passed?
        const u32 absFrameTime_ms = currKeyFrame.relTime_ms + _startTime_ms;
        if(HAL::GetTimeStamp() >= absFrameTime_ms)
        {
          // We are transitioning out of this keyframe and into the next
          // one in the list (if there is one).
          currKeyFrame.TransitionOutOf(_startTime_ms);
          
          ++subSystem.currFrame;
          
          if(subSystem.currFrame < subSystem.numFrames) {
            PRINT("Moving to keyframe %d of %d in subsystem %d\n", subSystem.currFrame+1, subSystem.numFrames, iSubSystem);
            KeyFrame& nextKeyFrame = subSystem.frames[subSystem.currFrame];
            nextKeyFrame.TransitionInto(_startTime_ms);
          } else {
            PRINT("Subsystem %d finished all %d of its frames\n", iSubSystem, subSystem.numFrames);
            subSystemPlaying[iSubSystem] = false;
          }
          
        } // if GetTimeStamp() >= absFrameTime_ms
      } // if/else subSystem.numFrames == 0
      
    } // for each subsystem
    
    // isPlaying should be true after this loop if any of the subSystems are still
    // playing. False otherwise.
    _isPlaying = false;
    for(s32 iSubSystem=0; iSubSystem<NUM_SUBSYSTEMS; ++iSubSystem) {
      _isPlaying |= subSystemPlaying[iSubSystem];
    }
    
    if(!_isPlaying) {
      PRINT("No subsystems in animation %d still playing. Stopping.\n", _ID);
      Stop();
    }
    
  } // Update
  
  
  void Animation::Stop()
  {
    /*
    if(_returnToOrigState) {
      HeadController::SetAngleRad(_origHeadAngle);
      LiftController::SetDesiredHeight(_origLiftHeight);
    }
     */
    
    for(s32 iSubSystem = 0; iSubSystem < NUM_SUBSYSTEMS; ++iSubSystem)
    {
      _subSystems[iSubSystem].frames[_subSystems[iSubSystem].currFrame].Stop();
    }
    
    _isPlaying = false;
    
    PRINT("Stopped playing animation %d.\n", _ID);
  }
  
  
  bool Animation::IsDefined()
  {
    for(s32 iSubSystem = 0; iSubSystem < NUM_SUBSYSTEMS; ++iSubSystem)
    {
      if(_subSystems[iSubSystem].numFrames > 0) {
        return true;
      }
    }
    
    return false;
  }
  
  
  void Animation::Clear()
  {
    for(s32 iSubSystem = 0; iSubSystem < NUM_SUBSYSTEMS; ++iSubSystem)
    {
      // TODO: Make a Clear() method in KeyFrame and call that here:
      _subSystems[iSubSystem].currFrame = 0;
      _subSystems[iSubSystem].numFrames = 0;
    }
  }
  
  Animation::SubSystems Animation::GetSubSystem(const KeyFrame::Type kfType)
  {
    switch(kfType)
    {
      case KeyFrame::HEAD_ANGLE:
      case KeyFrame::START_HEAD_NOD:
      case KeyFrame::STOP_HEAD_NOD:
        return HEAD;
        
      case KeyFrame::LIFT_HEIGHT:
      case KeyFrame::START_LIFT_NOD:
      case KeyFrame::STOP_LIFT_NOD:
        return LIFT;
        
      case KeyFrame::DRIVE_LINE_SEGMENT:
      case KeyFrame::DRIVE_ARC:
      case KeyFrame::BACK_AND_FORTH:
      case KeyFrame::START_WIGGLE:
      case KeyFrame::POINT_TURN:
      //case KeyFrame::STOP_WIGGLE:
        return POSE;
        
      case KeyFrame::SET_LED_COLORS:
      case KeyFrame::EYE_BLINK:
        return LIGHTS;
        
      case KeyFrame::PLAY_SOUND:
        return SOUND;
        
      default:
        PRINT("Unknown subsystem for KeyFrame type %d.\n", kfType);
        return NUM_SUBSYSTEMS;
        
    } // switch(kfType)
    
  } // GetSubSystem()
  
  
  
  Result Animation::AddKeyFrame(const KeyFrame& keyframe)
  {
    const SubSystems whichSubSystem = Animation::GetSubSystem(keyframe.type);
    
    Animation::KeyFrameList& animSubSystem = _subSystems[whichSubSystem];
    
    AnkiConditionalErrorAndReturnValue(animSubSystem.numFrames < Animation::MAX_KEYFRAMES,
                                       RESULT_FAIL,
                                       "Animation.AddKeyFrame.TooManyFrames",
                                       "No more space to add given keyframe to specified animation subsystem.\n");
    
    animSubSystem.frames[animSubSystem.numFrames++] = keyframe;
    
    PRINT("Added frame %d to subsystem %d of animation %d\n",
          animSubSystem.numFrames, whichSubSystem, _ID);
    
    return RESULT_OK;
  }

  
} // namespace Cozmo
} // namespace Anki

