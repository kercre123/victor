/*
 * File: engineAnimationController.h
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#ifndef __Basestation_Animations_EngineAnimationController_H__
#define __Basestation_Animations_EngineAnimationController_H__

#include "anki/common/types.h"

#include "anki/cozmo/basestation/animations/animationControllerTypes.h"


#include "util/helpers/noncopyable.h"
#include "util/signals/signalHolder.h"
#include <memory>

namespace Anki {
namespace Cozmo {
  
class CannedAnimationContainer;
class AnimationGroupContainer;
class CozmoContext;
class Robot;

namespace Audio {
class AudioMixingConsole;
class RobotAudioOutputSource;
class RobotAudioClient;
}
namespace ExternalInterface {
struct PlayAnimation_DEV;
}
namespace RobotAnimation {
  
class AnimationFrameInterleaver;
class AnimationMessageBuffer;
class AnimationSelector;
class StreamingAnimation;
  
  
class EngineAnimationController : public Util::noncopyable, Util::SignalHolder {
  
public:
  
  EngineAnimationController(const CozmoContext* context, Audio::RobotAudioClient* audioClient);
  
  ~EngineAnimationController();
  
  Result Update(Robot& robot);
  
  
  const CozmoContext* GetContext() const { return _context; }
  const Audio::RobotAudioClient* GetAudioClient() const { return _audioClient; }
  
  
private:
  
  const CozmoContext* _context = nullptr;
  
  // Audio Classes
  Audio::RobotAudioClient* _audioClient = nullptr;
  
  // Note: This is ticked by FrameInterleaver
  std::unique_ptr<Audio::AudioMixingConsole>  _audioMixer;
  
  // Animation Service Classes
  std::unique_ptr<AnimationSelector>          _animationSelector;
  std::unique_ptr<AnimationFrameInterleaver>  _frameInterleaver;
  std::unique_ptr<AnimationMessageBuffer>     _messageBuffer;
  
  
  // Container for all known "canned" animations (i.e. non-live)
  CannedAnimationContainer& _animationContainer;
  AnimationGroupContainer&  _animationGroups;
  
  // FIXME: TEST CODE
  void HandleMessage(const ExternalInterface::PlayAnimation_DEV& msg);
  
};
  
  
} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki

#endif /* __Basestation_Animations_EngineAnimationController_H__ */
