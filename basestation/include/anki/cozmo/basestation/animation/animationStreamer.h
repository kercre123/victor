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

#include "anki/types.h"
#include "anki/cozmo/basestation/cannedAnimationContainer.h"
#include "anki/cozmo/basestation/utils/hasSettableParameters.h"
#include "clad/types/liveIdleAnimationParameters.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {
  
  // Forward declaration
  class Robot;
  
  class IExternalInterface;
  namespace ExternalInterface {
    class MessageGameToEngine;
  }
  
  class AnimationStreamer : public HasSettableParameters<LiveIdleAnimationParameter, ExternalInterface::MessageGameToEngineTag::SetLiveIdleAnimationParameters, f32>
  {
  public:
    static const std::string LiveAnimation;
    static const std::string AnimToolAnimation;
    static const u8          IdleAnimationTag = 255;
    
    AnimationStreamer(IExternalInterface* externalInterface, CannedAnimationContainer& container);
    
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
    
    // Required by HasSettableParameters:
    virtual void SetDefaultParams() override;
    
  private:
    
    Result InitStream(Animation* anim, u8 withTag);
    Result Stream(Robot& robot, Animation* anim);
    bool IsFinished(Animation* anim) const;
    
    Result UpdateLiveAnimation(Robot& robot);
    
    CannedAnimationContainer& _animationContainer;

    Animation*  _idleAnimation; // default points to "live" animation
    Animation*  _streamingAnimation;
    TimeStamp_t _timeSpentIdling_ms;
    
    bool _isIdling;
    
    u32 _numLoops;
    u32 _loopCtr;
    u8  _tagCtr;
    
    bool _startOfAnimationSent;
    
    // When this animation started playing (was initialized) in milliseconds, in
    // "real" basestation time
    TimeStamp_t _startTime_ms;
    
    // Where we are in the animation in terms of what has been streamed out, since
    // we don't stream in real time. Each time we send an audio frame to the
    // robot (silence or actual audio), this increments by one audio sample
    // length, since that's what keeps time for streaming animations (not a
    // clock)
    TimeStamp_t _streamingTime_ms;
    
    bool _endOfAnimationSent;
    
    // TODO: Remove these once we aren't playing robot audio on the device
    TimeStamp_t _playedRobotAudio_ms;
    std::deque<const RobotAudioKeyFrame*> _onDeviceRobotAudioKeyFrameQueue;
    const RobotAudioKeyFrame* _lastPlayedOnDeviceRobotAudioKeyFrame;
    
    bool BufferMessageToSend(RobotInterface::EngineToRobot* msg);
    Result SendBufferedMessages(Robot& robot);
    
    std::list<RobotInterface::EngineToRobot*> _sendBuffer;
    s32 _numBytesToSend;
    uint8_t _tag;
    
    // Send larger keyframes "hot" for reliable transport (this includes
    // audio samples and face images)
    static const bool SEND_LARGE_KEYFRAMES_HOT = false;
    
    // "Flow control" for not overrunning reliable transport in a single
    // update tick
    static const s32 MAX_BYTES_FOR_RELIABLE_TRANSPORT;
    
    Util::RandomGenerator _rng;
    
    // For live animation
    Animation      _liveAnimation;
    bool           _isLiveTwitchEnabled;
    s32            _nextBlink_ms;
    s32            _nextLookAround_ms;
    s32            _bodyMoveDuration_ms;
    s32            _liftMoveDuration_ms;
    s32            _headMoveDuration_ms;
    s32            _bodyMoveSpacing_ms;
    s32            _liftMoveSpacing_ms;
    s32            _headMoveSpacing_ms;
    
  }; // class AnimationStreamer
  
} // namespace Cozmo
} // namespace Anki

#endif // ANKI_COZMO_ANIMATION_STREAMER_H