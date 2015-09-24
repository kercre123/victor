/**
 * File: animationStreamer.h
 *
 * Authors: Andrew Stein
 * Created: 2015-06-25
 *
 * Description: 
 * 
 *   Handles streaming a given animation from a CannedAnimationContainer
 *   to a robot.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef ANKI_COZMO_ANIMATION_STREAMER_H
#define ANKI_COZMO_ANIMATION_STREAMER_H

#include "anki/cozmo/shared/cozmoTypes.h"

#include "anki/cozmo/basestation/cannedAnimationContainer.h"

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class Robot;
 
  class AnimationStreamer
  {
  public:
    static const std::string LiveAnimation;
    static const u8          IdleAnimationTag = 255;
    
    AnimationStreamer(CannedAnimationContainer& container);
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use numLoops = 0 to play the animation indefinitely.
    // Returns a tag you can use to monitor whether the robot is done playing this
    // animation.
    // Actual streaming occurs on calls to Update().
    u8 SetStreamingAnimation(const std::string& name, u32 numLoops = 1);
    
    // Sets the "idle" animation that will be streamed (in a loop) when no other
    // animation is streaming. Use empty string ("") to disable.
    // Use static LiveAnimation above to use live procedural animation (default).
    Result SetIdleAnimation(const std::string& name);
    
    // If any animation is set for streaming and isn't done yet, stream it.
    Result Update(Robot& robot);
    
    // Returns true if the idle animation is playing
    bool IsIdleAnimating() const;
    
    const std::string GetStreamingAnimationName() const;
    
  private:
    
    Result UpdateLiveAnimation(Robot& robot);
    
    CannedAnimationContainer& _animationContainer;

    Animation      _liveAnimation;

    Animation*  _idleAnimation; // default points to "live" animation
    Animation*  _streamingAnimation;
    TimeStamp_t _timeSpentIdling_ms;
    
    bool _isIdling;
    
    u32 _numLoops;
    u32 _loopCtr;
    u8  _tagCtr;
    
    Util::RandomGenerator _rng;
    
    // For live animation
    s32 _nextBlink_ms;
    s32 _nextLookAround_ms;
    s32 _bodyMoveDuration_ms;
    s32 _liftMoveDuration_ms;
    s32 _headMoveDuration_ms;
    
  }; // class AnimationStreamer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIMATION_STREAMER_H