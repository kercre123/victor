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
    AnimationStreamer(CannedAnimationContainer& container);
    
    // Sets an animation to be streamed and how many times to stream it.
    // Use zero to play the animation indefinitely.
    // Actual streaming occurs on calls to Update().
    Result SetStreamingAnimation(const std::string& name, u32 numLoops = 1);
    
    // If any animation is set for streaming and isn't done yet, stream it.
    Result Update(Robot& robot);
    
  private:
    
    CannedAnimationContainer& _animationContainer;
    
    Animation* _streamingAnimation;
    
    u32 _numLoops;
    u32 _loopCtr;
    
  }; // class AnimationStreamer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIMATION_STREAMER_H