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
    HEAD             = 0,
    LIFT             = 1,
    POSE             = 2,
    SOUND            = 3,
    LIGHTS           = 4,
    NUM_SUBSYSTEMS   = 5
  };

  // Methods for controlling playback:
  
  void Init();
  
  void Update();
  
  void Stop();
  
  bool IsPlaying() const;
  
  // Methods for defining animations:
  void Clear();
  void SetID(AnimationID_t newID);
  Result AddKeyFrame(const KeyFrame&             keyframe,
                     const Animation::SubSystems whichSubSystem);
  
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
    s32      numFrames;
    s32      currFrame;
  } _subSystems[NUM_SUBSYSTEMS];
  
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

inline bool Animation::IsPlaying() const {
  return _isPlaying;
}
  
inline void Animation::SetID(AnimationID_t newID) {
  _ID = newID;
}
  
} //  namespace Cozmo
} //  namespace Anki

#endif // ANKI_COZMO_ROBOT_ANIMATION_H

