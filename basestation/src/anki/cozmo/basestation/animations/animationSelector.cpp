/*
 * File: animationSelector.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/animation/animation.h"
#include "anki/cozmo/basestation/animations/animationSelector.h"
#include "anki/cozmo/basestation/animations/streamingAnimation.h"
//#include "anki/cozmo/basestation/audio/robotAudioClient.h"


namespace Anki {
namespace Cozmo {
namespace RobotAnimation {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
AnimationSelector::AnimationSelector(Audio::RobotAudioClient& audioClient)
: _audioClient(audioClient)
{
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationSelector::BufferStreamingAnimation(Animation* animation)
{
  // Create & Store Streaming Animations by copying original animation
  DEV_ASSERT(animation != nullptr, "AnimationSelector::BufferStreamingAnimation.Animation.IsNull");
  
  // FIXME: Need to set correct state
  const Audio::RobotAudioClient::RobotAudioOutputSource output = _audioClient.GetOutputSource();
  
  // check if Animation is already in cache
  const auto findId = _animationMap.find(animation->GetName());
  if (findId != _animationMap.end()) {
    // Animation already in cache
    
    // Check if it has the same output source we
    if (findId->second.outputSource == output) {
      // All good, already have animation cached
      return false;
    }
    // Else
    // Need to re-create animation with updated audio source
    _animationMap.erase(findId);
  }
  
  // Create Streaming animation
  AnimationInfo info(std::unique_ptr<StreamingAnimation>(new StreamingAnimation(*animation)), output);
  auto animationIt = _animationMap.emplace(animation->GetName(), std::move(info));
  AnimationInfo& animationInfo = animationIt.first->second;
  DEV_ASSERT(animationIt.second, "AnimationSelector.BufferStreamingAnimation._animationMap.emplace.false");
  
  // Start Generating audio
  BufferAnimationAudio(animationInfo);
  
  // Return Key
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool AnimationSelector::IsAnimationBuffering(const std::string& animationName) const
{
  bool isBuffering = false;
  auto animationIt = _animationMap.find(animationName);
  if (animationIt != _animationMap.end()) {
    isBuffering = animationIt->second.isBuffering;
  }
  return isBuffering;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingAnimation* AnimationSelector::GetStreamingAnimation(const std::string& animationName) const
{
  StreamingAnimation* anim = nullptr;
  auto animationIt = _animationMap.find(animationName);
  if (animationIt != _animationMap.end()) {
    if (animationIt->second.isBuffering) {
      anim = animationIt->second.animation.get();
    }
  }
  
  return anim;
}

void AnimationSelector::RemoveStreamingAnimation(const std::string& animationKey)
{
  _animationMap.erase(animationKey);
}
  

// Private
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void AnimationSelector::BufferAnimationAudio(AnimationInfo& animationInfo)
{
  // TODO: Need to be able to retry if all buffer pool is empty
  
  
  // Only call this once per streaming animation
  DEV_ASSERT(!animationInfo.isBuffering, "AnimationSelector.BufferAnimationAudio.isBuffering.true");
  // Check output Source type
  switch (animationInfo.outputSource) {
    case Audio::RobotAudioClient::RobotAudioOutputSource::None:
    {
      // Should never be in this state
      DEV_ASSERT(animationInfo.outputSource != Audio::RobotAudioClient::RobotAudioOutputSource::None,
                 "AnimationSelector.BufferAnimationAudio.outputSource.None");
      break;
    }
      
    case Audio::RobotAudioClient::RobotAudioOutputSource::PlayOnDevice:
    {
      // Generate Animation's Audio Event List
      animationInfo.animation->GenerateAudioEventList(_audioClient.GetRandomGenerator());
      
      // FIXME: Need to create device playback stuff!!
      animationInfo.animation->GenerateDeviceAudioEvents(&_audioClient);
      
      animationInfo.isBuffering = true;
      break;
    }
      
    case Audio::RobotAudioClient::RobotAudioOutputSource::PlayOnRobot:
    {
      if (_audioClient.AvailableGameObjectAndAudioBufferInPool()) {
        // Generate Animation's Audio Event List
        animationInfo.animation->GenerateAudioEventList(_audioClient.GetRandomGenerator());
        // Start buffering
        animationInfo.animation->GenerateAudioData(&_audioClient);
        
        animationInfo.isBuffering = true;
      }
      else {
        PRINT_NAMED_WARNING("AnimationSelector.BufferAnimationAudio",
                            "RobotAudioClient.AvailableGameObjectAndAudioBufferInPool.IsFalse");
      }
      
      break;
    }
  }
  
  
}

} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki
