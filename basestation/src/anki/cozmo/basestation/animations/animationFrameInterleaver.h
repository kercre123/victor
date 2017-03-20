/*
 * File: animationFrameInterleaver.h
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_AnimationFrameInterleaver_H__
#define __Basestation_Animations_AnimationFrameInterleaver_H__


#include "anki/cozmo/basestation/animations/animationControllerTypes_Internal.h"
#include "anki/cozmo/basestation/animations/animationSelector.h"

#include <cstdint>
#include <functional>
#include <vector>
#include <queue>

namespace Anki {
namespace Cozmo {
  
namespace Audio {
class AudioMixingConsole;
class RobotAudioOutputSource;
}

namespace RobotInterface {
class EngineToRobot;
}
  
namespace RobotAnimation {
  
class StreamingAnimation;
class AnimationAudioInputSource;
  
  
class AnimationFrameInterleaver {
  
public:
  
  enum class State {
    NoAnimations = 0,
    BufferingAnimation,
    PlayingAnimations,
    TransitioningBetweenAnimations
  };
  
  
  AnimationFrameInterleaver(Audio::AudioMixingConsole& audioMixer);
  
  State GetState() const { return _state; }
  
  
  // NOTE: Animation must be Init "reset or NotPlaying" before set as next animation
  // Return false if there is already a "next" animation in the queue. Only track current playing and next animation
  // FIXME: Need to be able to inturpt current stack, pop back animation and replace with "new" nest animation
  bool SetNextAnimation(StreamingAnimation* animation,
                        uint32_t fadeDuration_ms = 0,
                        bool abort = false);

  // Every tick call update to update current state
  State Update();

  void PopFrameRobotMessages(EngineToRobotMessageList& out_msgList);
  
  
  // Update Robot Master Volume
  void SetRobotMasterVolume(float volume);
  
  // TODO: Remove this
  using AnimationCompletedCallbackFunc = std::function<void(const std::string& completedAnimation)>;
  void SetAnimationCompletedCallback(AnimationCompletedCallbackFunc callback) { _animationCompletedCallback = callback; }
  
  
private:
  
  struct AnimationPlaybackInfo
  {
    StreamingAnimation* animation = nullptr;
    AnimationAudioInputSource* audioInput = nullptr;
    uint32_t fadeDuration_ms = 0;
    bool abort = false;
    
    AnimationPlaybackInfo(StreamingAnimation* animation,
                          AnimationAudioInputSource* audioInput,
                          uint32_t fadeDuration_ms,
                          bool abort)
    : animation(animation)
    , audioInput(audioInput)
    , fadeDuration_ms(fadeDuration_ms)
    , abort(abort)
    { }                      
  };
  
  enum class AnimationPositionState {
    Beginning,
    Middle,     // Rename this, I don't like it!!!
    End
  };
  
  
  Audio::AudioMixingConsole& _audioMixer;
  
  State _state = State::NoAnimations;
  
  // Current animations to play
  std::queue<AnimationPlaybackInfo> _playbackQueue;
  
  // Available Audio input
  std::queue<AnimationAudioInputSource*> _audioInputs;
  
  Audio::RobotAudioOutputSource*  _robotAudioOutput = nullptr;
  
  AnimationCompletedCallbackFunc _animationCompletedCallback = nullptr;
  
  
  // Figure out what state interleaver is in
  void CalculateCurrentState();
  
  
  // Pop the current andimation and put audio input back into pool
  // Note: Always call this when removing from _playbackQueue
  void PopCurrentAnimation();
  
  bool IsAnimationInQueue() const { return !_playbackQueue.empty(); }
  
  AnimationPlaybackInfo* const GetCurrentAnimationInfo();
  AnimationPlaybackInfo* const GetNextAnimationInfo();
  
  // Converte animation frames into robot messages
  void CreateAnimationMessages(AnimationPositionState positionState,
                               KeyframeList &keyframeList,
                               EngineToRobotMessageList &out_msgList);
};
  
} // namespace RobotAnimation
} // namespace Cozmo
} // namespace Anki


#endif /* __Basestation_Animations_AnimationFrameInterleaver_H__ */
