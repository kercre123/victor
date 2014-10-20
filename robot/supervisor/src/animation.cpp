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
  
  void Animation::Init()
  {
    PRINT("Init Animation %d\n", _ID);
    
    _startTime_ms      = HAL::GetTimeStamp();
    
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
    
    for(s32 iSubSystem=0; iSubSystem<NUM_SUBSYSTEMS; ++iSubSystem) {
      _subSystems[iSubSystem].currFrame = 0;
    }
    
  } // Init()
  
  
  void Animation::Update()
  {
    bool subSystemPlaying[NUM_SUBSYSTEMS];
    
    for(s32 iSubSystem=0; iSubSystem<NUM_SUBSYSTEMS; ++iSubSystem)
    {
      // Get a reference to the current subSystem, for convenience
      Animation::KeyFrameList& subSystem = _subSystems[iSubSystem];

      if(subSystem.numFrames == 0) {
        // Skip empty subsystems
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
            PRINT("Moving to keyframe %d of %d in subsystem %d\n", subSystem.currFrame, subSystem.numFrames, iSubSystem);
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
    
    _isPlaying = false;
    
    PRINT("Stopped playing animation %d.\n", _ID);
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
  
  Result Animation::AddKeyFrame(const KeyFrame&             keyframe,
                                const Animation::SubSystems whichSubSystem)
  {
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

