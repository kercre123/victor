/*
 * File: animationSelector.h
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 *   NOTE: StreamingAnimation are stored with shared pointers their for they are only deleted from memory when all shared
 *         pointers of that objected are deleted. This lets us remove them from the AnimationSelector class without
 *         deleting them so other stages of the animation process can use the StreamingAnimation
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_AnimationSelector_H__
#define __Basestation_Animations_AnimationSelector_H__

#include "anki/cozmo/basestation/animations/animationControllerTypes_Internal.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
#include <memory>
#include <string>
#include <unordered_map>


namespace Anki {
namespace Cozmo {

namespace Audio {
class RobotAudioClient;
}

class Animation;
  
namespace RobotAnimation {
class StreamingAnimation;


class AnimationSelector {
  
public:
  
  AnimationSelector(Audio::RobotAudioClient& audioClient);
  
  // Return: True if added to buffer, false if animation with key already existed
  bool BufferStreamingAnimation(Animation* animation);
  
  // Check if the animation started to buffer before using it
  // Return False if not buffer has not started or is not found
  bool IsAnimationBuffering(const std::string& animationName) const;
  
  // Return: nullptr if animation is not found or did not start to buffer yet
  StreamingAnimation* GetStreamingAnimation(const std::string& animationName) const;
  
  void RemoveStreamingAnimation(const std::string& animationName);
  
private:
  
  
  struct AnimationInfo {
    std::unique_ptr<StreamingAnimation> animation;
    bool isBuffering = false;
    Audio::RobotAudioClient::RobotAudioOutputSource outputSource = Audio::RobotAudioClient::RobotAudioOutputSource::None;
    // TODO: Add Output State
    AnimationInfo(std::unique_ptr<StreamingAnimation>&& animation, Audio::RobotAudioClient::RobotAudioOutputSource outputSource)
    : animation(std::move(animation))
    , outputSource(outputSource)
    { }
  };
  
  Audio::RobotAudioClient& _audioClient;
  
  std::unordered_map<std::string, AnimationInfo> _animationMap;
  
  void BufferAnimationAudio(AnimationInfo& animationInfo);
  
};
  
} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki


#endif /* __Basestation_Animations_AnimationSelector_H__ */
