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

  void Update();
  
  void SetAnimationStreamer(AnimationStreamer* streamer)
  {
    _streamer = streamer;
  }

  // Most functions here need to be thread safe due to them being called from trigger word detected
  // callbacks which happen on a separate thread
  
  void SetTriggerWordResponse(const RobotInterface::SetTriggerWordResponse& msg);
  
  // Start the robot's response to the trigger in order to indicate that the robot may be streaming audio
  // The GetInAnimation is optional, the earcon and backpack lights are not
  using OnTriggerAudioCompleteCallback = std::function<void(bool)>;
  void SetPendingTriggerResponseWithGetIn(OnTriggerAudioCompleteCallback = {});
  void SetPendingTriggerResponseWithoutGetIn(OnTriggerAudioCompleteCallback = {});

  // Indicates whether or not the audio stream state manager will be able to indicate to the user
  // that streaming may be happening - if this returns false 
  bool HasValidTriggerResponse();

  // Indicates whether voice data should be streamed to the cloud after the trigger response has indicated to
  // the user that streaming may be happening
  bool ShouldStreamAfterTriggerWordResponse();
  
  bool ShouldSimulateStreamAfterTriggerWord();

private:

  void StartTriggerResponseWithGetIn(OnTriggerAudioCompleteCallback = {});
  void StartTriggerResponseWithoutGetIn(OnTriggerAudioCompleteCallback = {});


  const AnimContext* _context = nullptr;
  AnimationStreamer* _streamer = nullptr;

  Anki::AudioEngine::Multiplexer::PostAudioEvent _postAudioEvent;
  bool _shouldTriggerWordStartStream;
  bool _shouldTriggerWordSimulateStream;
  uint8_t _getInAnimationTag;
  std::string _getInAnimName;

  // Trigger word responses are triggered via callbacks from the trigger word detector thread
  // so we need to be thread safe and have pending responses to be executed on the main thread in Update
  std::recursive_mutex _triggerResponseMutex;
  bool _havePendingTriggerResponse = false;
  bool _pendingTriggerResponseHasGetIn = false;
  OnTriggerAudioCompleteCallback _responseCallback;
};

} // namespace Vector
} // namespace Anki

#endif  // #ifndef COZMO_ANIM_SHOW_AUDIO_STREAM_STATE_MANAGER_H
