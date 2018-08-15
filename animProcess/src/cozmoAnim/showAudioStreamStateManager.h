/**
* File: cozmoAnim/showAudioStreamStateManager.h
*
* Author: Kevin M. Karol
* Created: 8/3/2018
*
* Description: Communicates the current state of cloud audio streaming to the user
* and ensures expectations of related animation components are met (e.g. motion/lack there of when streaming)
*
* Copyright: Anki, Inc. 2018
**/

#ifndef COZMO_ANIM_SHOW_AUDIO_STREAM_STATE_MANAGER_H
#define COZMO_ANIM_SHOW_AUDIO_STREAM_STATE_MANAGER_H

#include "clad/robotInterface/messageEngineToRobot.h"
#include "coretech/common/shared/types.h"
#include "cozmoAnim/animContext.h"

namespace Anki {
namespace Vector {

class AnimationStreamer;

namespace Audio {
class EngineRobotAudioInput;
}

class ShowAudioStreamStateManager{
public:

  ShowAudioStreamStateManager(const AnimContext* context);
  virtual ~ShowAudioStreamStateManager();

  void SetAnimationStreamer(AnimationStreamer* streamer)
  {
    _streamer = streamer;
  }

  void SetAudioInput(Audio::EngineRobotAudioInput* audioInput)
  {
    _audioInput = audioInput;
  }

  void SetTriggerWordResponse(const RobotInterface::SetTriggerWordResponse& msg);
  
  // Start the robot's response to the trigger in order to indicate that the robot may be streaming audio
  // The GetInAnimation is optional, the earcon and backpack lights are not
  void StartTriggerResponseWithGetIn();
  void StartTriggerResponseWithoutGetIn();

  // Indicates whether or not the audio stream state manager will be able to indicate to the user
  // that streaming may be happening - if this returns false 
  bool HasValidTriggerResponse() const;

  // Indicates whether voice data should be streamed to the cloud after the trigger response has indicated to
  // the user that streaming may be happening
  bool ShouldStreamAfterTriggerWordResponse() const;

private:
  const AnimContext* _context = nullptr;
  AnimationStreamer* _streamer = nullptr;
  Audio::EngineRobotAudioInput* _audioInput = nullptr;
  
  Anki::AudioEngine::Multiplexer::PostAudioEvent _postAudioEvent;
  bool _shouldTriggerWordStartStream;
  uint8_t _getInAnimationTag;
  std::string _getInAnimName;
};

} // namespace Vector
} // namespace Anki

#endif  // #ifndef COZMO_ANIM_SHOW_AUDIO_STREAM_STATE_MANAGER_H
