/**
 * File: animation.h
 *
 * Author: Andrew Stein
 * Created: 10/16/2014
 *
 * Description:
 *
 *   Defines an animation, which consists of a list of keyframes for each 
 *   animate-able subsystem, to be played concurrently. Note that a subsystem 
 *   can have no keyframes (numFrames==0) and it will simply be ignored (and 
 *   left unlocked) during that animation.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef ANKI_COZMO_ROBOT_ANIMATION_H
#define ANKI_COZMO_ROBOT_ANIMATION_H

#include "keyFrame.h"
#include "anki/cozmo/shared/cozmoTypes.h"

namespace Anki {
namespace Cozmo {

  class Animation
  {
  public:
    
    enum SubSystems {
      HEAD = 0,
      LIFT,
      POSE,
      SOUND,
      LIGHTS, // TODO: Separate eyes from other lights eventually?
      NUM_SUBSYSTEMS
    };
    
    Animation();
    
    //
    // Methods for controlling playback:
    //
    
    // Called each time an animatin is about to be played
    void Init();
    
    // Called while the animation is playing
    void Update();
    
    // Stop the animatin prematurely (called internally when an animation completes)
    void Stop();
    
    // Query whether the animation is currently playing
    bool IsPlaying() const;
    
    //
    // Methods for defining animations:
    //
    
    // True if any subsystems have frames defined.
    bool IsDefined();
    void Clear();
    void SetID(AnimationID_t newID);
    Result AddKeyFrame(const KeyFrame& keyframe);
    
  private:
    
    static const s32 MAX_KEYFRAMES = 32; // per subsystem
    
    AnimationID_t _ID; // needed, or simply the slot it's stored in?
    
    // what time the animation started, relative to which each keyFrame's time
    // is specified
    TimeStamp_t _startTime_ms;
    
    bool _isPlaying;
    
    // Lists of keyframes for each subsystem
    struct KeyFrameList {
      KeyFrame frames[MAX_KEYFRAMES];
      bool     isReady;   // true once first frame is "in position"
      s32      numFrames;
      s32      currFrame;
      
      KeyFrameList();
    } _subSystems[NUM_SUBSYSTEMS];
    
    static SubSystems GetSubSystem(const KeyFrame::Type kfType);
    
    bool _allSubSystemsReady;
    bool CheckSubSystemReadiness();
    
    /*
     // Flag to return subsystems to their original state when the animation is
     // complete or is stopped/interrupted. Note that this does not apply to
     // body pose, since there is no localization during animation.
     bool _returnToOrigState;
     
     // Place to store original states for each subsystem, used when
     // returnToOrigState is true.
     // TODO: Could probably store original angle/height in u8 (don't need so much resolution)
     f32       _origHeadAngle;
     f32       _origLiftHeight;
     u32       _origLEDcolors[HAL::NUM_LEDS];
     SoundID_t _origSoundID;
     */
    
  }; // class Animation
  
  
  //
  // Inline Implementations
  //
  
  inline bool Animation::IsPlaying() const {
    return _isPlaying;
  }
  
  inline void Animation::SetID(AnimationID_t newID) {
    _ID = newID;
  }
  
} //  namespace Cozmo
} //  namespace Anki

#endif // ANKI_COZMO_ROBOT_ANIMATION_H

