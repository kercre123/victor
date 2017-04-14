/*
 * File: animationFrameInterleaver.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/animations/animationFrameInterleaver.h"
#include "anki/cozmo/basestation/animations/streamingAnimation.h"
#include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"

#include "anki/cozmo/basestation/animations/animationAudioInputSource.h"

#include "anki/cozmo/basestation/audio/robotAudioOutputSource.h"

#include "clad/robotInterface/messageEngineToRobot.h"

using namespace Anki::Cozmo::RobotInterface;

namespace Anki {
namespace Cozmo {
namespace RobotAnimation {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationFrameInterleaver::AnimationFrameInterleaver(Audio::AudioMixingConsole& audioMixer)
: _audioMixer(audioMixer)
{
  // Setup audio
  using namespace Audio;
  // Setup Audio Input source pool
  const size_t audioInputPoolSize = 2;
  for (size_t idx = 0; idx < audioInputPoolSize; ++idx) {
    AnimationAudioInputSource* audioInput = new AnimationAudioInputSource(_audioMixer);
    // Transfer ownership to Audio Mixer
    _audioMixer.AddInputSource(audioInput);
    // Add Inputs to pool
    _audioInputs.push(audioInput);
  }
  
  // Setup Audio Output source
  _robotAudioOutput = new Audio::RobotAudioOutputSource(_audioMixer);
  // Transfer ownership to Audio Mixer
  _audioMixer.AddOutputSource(_robotAudioOutput);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationFrameInterleaver::SetNextAnimation(StreamingAnimation* animation,
                                                 uint32_t fadeDuration_ms,
                                                 bool abort)
{
  DEV_ASSERT(animation != nullptr,
             "AnimationFrameInterleaver.SetNextAnimation.StreamingAnimationWeakPtr.Expired");
  DEV_ASSERT(!animation->DidStartPlaying(),
             "AnimationFrameInterleaver.SetNextAnimation.StreamingAnimation.DidStartPlaying");
  if (_playbackQueue.size() >= 2) {
    return false;
  }
  
  DEV_ASSERT(!_audioInputs.empty(), "AnimationFrameInterleaver.SetNextAnimation.AudioInputs.IsEmpty");
  
  // Add to queue
  _playbackQueue.emplace(animation, _audioInputs.front(), fadeDuration_ms, abort);
  // Remove Audio Input from pool
  _audioInputs.pop();
  
  return true;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationFrameInterleaver::State AnimationFrameInterleaver::Update()
{
  // Update animations
  AnimationPlaybackInfo* animInfo = GetCurrentAnimationInfo();
  if (animInfo != nullptr) {
    animInfo->animation->Update();
    // Update both current & next animations
    animInfo = GetNextAnimationInfo();
    if (animInfo != nullptr) {
      animInfo->animation->Update();
    }
  }
  
  // Checks animation state and animation frame audio state
  CalculateCurrentState();
  
  return _state;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationFrameInterleaver::PopFrameRobotMessages(EngineToRobotMessageList& out_msgList)
{
  DEV_ASSERT(_state != State::BufferingAnimation,
             "AnimationFrameInterleaver.PopFrameRobotMessages.State.IsBufferingAnimation");
  
  
  // FIXME: Need to tick Audio Mixer but not other stuff
  if (_state == State::NoAnimations) {
    return;
  }
  
  // TODO: Insert Animation Mixer to get transition key frame sums
  const auto animInfo = GetCurrentAnimationInfo();
  DEV_ASSERT(animInfo != nullptr, "AnimationFrameInterleaver.PopFrameRobotMessages.AnimationInfo.IsNull");
  
  // Get Keyframes
  StreamingAnimationFrame aFrame;
  animInfo->animation->TickPlayhead(aFrame);
  
  // Produce animation keyframes & audio frame

  // Prepare Animation Audio
  animInfo->audioInput->SetNextFrame(aFrame.audioFrameData);
  
  // Tick Audio Mixer
  _audioMixer.ProcessFrame();
  
  // TODO: Get face layers for procedure face laying
  
  // TODO: Fix state when we are transitioning
  
  // Determine what part of the animation this frame is: beginning, middle or end
  AnimationPositionState positionState = AnimationPositionState::Middle;
  if (animInfo->animation->GetPlayheadFrame() == 1) {
    positionState = AnimationPositionState::Beginning;
  }
  else if (animInfo->animation->IsPlaybackComplete()) {
    positionState = AnimationPositionState::End;
  }
  
  CreateAnimationMessages(positionState, aFrame, out_msgList);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationFrameInterleaver::SetRobotMasterVolume(float volume)
{
  if (_robotAudioOutput != nullptr) {
    _robotAudioOutput->SetVolume(volume);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationFrameInterleaver::CalculateCurrentState()
{
  // If not playing frames, check if it's time for the next animation
  
  
  // If playing animation, check if completed and if a transition needs to happen
  
  const AnimationPlaybackInfo* animInfo = GetCurrentAnimationInfo();
  State newState = State::NoAnimations;
  switch (_state) {
    case State::NoAnimations:
    {
      // CORRECTION an animation could be put in the queue
      // If we are in this state we should NOT have an Animation Info
      // DEV_ASSERT(animInfo == nullptr, "AnimationFrameInterleaver.CalculateCurrentState.NoAnimations.AnimInfo.IsNotNull");
      // Check if there is an animations
      break;
    }
    case State:: BufferingAnimation:
    {
      // If we are in this state we should always have an Animation Info
      DEV_ASSERT(animInfo != nullptr, "AnimationFrameInterleaver.CalculateCurrentState.BufferingAnimation.InvalidInfo");
      // FIXME: Do Something here!!!
      break;
    }
      
    case State::PlayingAnimations:
    {
      // If we are in this state we should always have an Animation Info
      DEV_ASSERT(animInfo != nullptr, "AnimationFrameInterleaver.CalculateCurrentState.PlayingAnimations.InvalidInfo");
      // Check if still valid
      if (animInfo->animation->IsPlaybackComplete()) {
        // Pop animation
        const auto& animName = animInfo->animation->GetName();
        PopCurrentAnimation();
        if (_animationCompletedCallback != nullptr) {
          _animationCompletedCallback(animName);
        }
        // Check if there is another Animation in the queue
        animInfo = GetCurrentAnimationInfo();
      }
      
      // TODO: Check if we need to transition
      
      break;
    }
      
    case State::TransitioningBetweenAnimations:
    {
      // TODO: Check if we need to transition
      // If we are in this state we should always have an Animation Info
      DEV_ASSERT(animInfo != nullptr,
                 "AnimationFrameInterleaver.CalculateCurrentState.TransitioningBetweenAnimations.InvalidInfo");
      break;
    }
  }
  
  // Determine new state
  if (animInfo != nullptr) {
    const bool isReady = animInfo->animation->CanPlayNextFrame();
    newState = isReady ? State::PlayingAnimations : State::BufferingAnimation;
    
    // TODO: Need to do something different for transitioning Animations, be sure both animations are ready
  }
  else {
    newState = State::NoAnimations;
  }

  _state = newState;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationFrameInterleaver::PopCurrentAnimation()
{
  // Add audio input back into pool
  const auto& info = _playbackQueue.front();
  info.audioInput->ResetInput();
  _audioInputs.push(info.audioInput);
  // Pop animation
  _playbackQueue.pop();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationFrameInterleaver::AnimationPlaybackInfo* const AnimationFrameInterleaver::GetCurrentAnimationInfo()
{
  if (!_playbackQueue.empty()) {
    return &_playbackQueue.front();
  }
  
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationFrameInterleaver::AnimationPlaybackInfo* const AnimationFrameInterleaver::GetNextAnimationInfo()
{
  // There are only 2 animations in the queue
  if (_playbackQueue.size() > 1) {
    return &_playbackQueue.back();
  }
  
  return nullptr;
}

void AddKeyframeNonNullToList(IKeyFrame* keyframe, EngineToRobotMessageList &inOut_frameList)
{
  if (keyframe != nullptr) {
    const EngineToRobot * msg = keyframe->GetStreamMessage();
    if (msg != nullptr) {
      inOut_frameList.emplace_back(msg);
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationFrameInterleaver::CreateAnimationMessages(AnimationPositionState positionState,
                                                        StreamingAnimationFrame &frame,
                                                        EngineToRobotMessageList &out_msgList)
{
  // Convert animation keyframes into robot messages.
  // Note that message order is important!  Robot audio messages
  // must be first because they are used to synchronize frames.

  // Robot Audio
  const EngineToRobot* audioMsg = _robotAudioOutput->PopRobotAudioFrameMsg();
  out_msgList.emplace_back(audioMsg);
  
  // Animation Start?
  if (positionState == AnimationPositionState::Beginning) {
    // FIXME: Animation Tags
    out_msgList.emplace_back(new EngineToRobot(AnimKeyFrame::StartOfAnimation(20)));
  }
  
  // Device Audio
  AddKeyframeNonNullToList(frame.deviceAudioFrame, out_msgList);
  
  // Procedural Face
  //    auto& proceduralFaceTrack = GetTrack<ProceduralFaceKeyFrame>();
  //    if (proceduralFaceTrack.HasFramesLeft() &&
  //        proceduralFaceTrack.GetCurrentKeyFrame().IsTimeToPlay(_playheadTime_ms)) {
  //      // FIXME: No procedural face
  //      // Don't send procedure face
  //      proceduralFaceTrack.MoveToNextKeyFrame();
  //    }
  
  // Face Images
  AddKeyframeNonNullToList(frame.animFaceFrame, out_msgList);
  
  // Motors
  AddKeyframeNonNullToList(frame.headFrame, out_msgList);
  AddKeyframeNonNullToList(frame.liftFrame, out_msgList);
  AddKeyframeNonNullToList(frame.bodyFrame, out_msgList);
  
  // Event
  AddKeyframeNonNullToList(frame.eventFrame, out_msgList);
  
  // Backpack Lights
  AddKeyframeNonNullToList(frame.backpackFrame, out_msgList);
  
  // Animation End?
  if (positionState == AnimationPositionState::End) {
    out_msgList.emplace_back(new EngineToRobot(AnimKeyFrame::EndOfAnimation()));
  }
}


} // namespace RobotAnimation
} // namespace Cozmo
} // namespace Anki
