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

#include "anki/cozmo/basestation/animation/animationStreamer.h"
#include "anki/cozmo/basestation/animations/animationControllerTypes.h"

#include "util/helpers/noncopyable.h"
#include "util/signals/signalHolder.h"
#include <memory>

namespace Anki {
namespace Cozmo {
  
class AnimationGroupContainer;
class CannedAnimationContainer;
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
  
class EngineAnimationController :
  public IAnimationStreamer,
  public Util::noncopyable,
  Util::SignalHolder
{
  
public:
  
  EngineAnimationController(const CozmoContext* context, Audio::RobotAudioClient* audioClient);
  
  ~EngineAnimationController();
  
  // Sets an animation to be streamed and how many times to stream it.
  // Use numLoops = 0 to play the animation indefinitely.
  // Returns a tag you can use to monitor whether the robot is done playing this
  // animation.
  // If interruptRunning == true, any currently-streaming animation will be aborted.
  // Actual streaming occurs on calls to Update().
  Tag SetStreamingAnimation(Animation* anim, u32 numLoops = 1, bool interruptRunning = true) override;
  
  // Set the animation to be played when no other animation has been specified.  Use the empty string to
  // disable idle animation. NOTE: this wipes out any idle animation stack (from the push/pop actions below)
  Result SetIdleAnimation(AnimationTrigger animName) override;
  
  // Set the idle animation and also add it to the idle animation stack, so we can use pop later. The current
  // idle (even if it came from SetIdleAnimation) is always on the stack
  Result PushIdleAnimation(AnimationTrigger animName) override;
  
  // Return to the idle animation which was running prior to the most recent call to PushIdleAnimation.
  // Returns RESULT_OK on success and RESULT_FAIL if the stack of idle animations was empty.
  // Will not pop the last idle off the stack.
  Result PopIdleAnimation() override;
  
  // Add a procedural face "layer" to be combined with whatever is streaming
  Result AddFaceLayer(const std::string& name, FaceTrack&& faceTrack, TimeStamp_t delay_ms = 0) override;
  
  // Add a procedural face "layer" that is applied and then has its final
  // adjustment "held" until removed.
  // A handle/tag for the layer is returned, which is needed for removal.
  Tag AddPersistentFaceLayer(const std::string& name, FaceTrack&& faceTrack) override;
  
  // Remove a previously-added persistent face layer using its tag.
  // If duration > 0, that amount of time will be used to transition back
  // to no adjustment
  void RemovePersistentFaceLayer(Tag tag, s32 duration_ms = 0) override;
  
  // Add a keyframe to the end of an existing persistent face layer
  void AddToPersistentFaceLayer(Tag tag, ProceduralFaceKeyFrame&& keyframe) override;
  
  // Remove any existing procedural eye dart created by KeepFaceAlive
  void RemoveKeepAliveEyeDart(s32 duration_ms) override;
  
  // If any animation is set for streaming and isn't done yet, stream it.
  Result Update(Robot& robot) override;
  
  const Animation* GetStreamingAnimation() const override;
  
  const std::string& GetAnimationNameFromGroup(const std::string& name, const Robot& robot) const override;
  
  void SetDefaultParams() override;
  
protected:
  const CozmoContext* GetContext() const { return _context; }
  const Audio::RobotAudioClient* GetAudioClient() const { return _audioClient; }
  
private:
  using RobotAudioClient = Audio::RobotAudioClient;
  using AudioMixingConsole = Audio::AudioMixingConsole;
  using PlayAnimation_DEV = ExternalInterface::PlayAnimation_DEV;
  
  const CozmoContext* _context = nullptr;
  
  // Audio Classes
  RobotAudioClient* _audioClient = nullptr;
  
  // Note: This is ticked by FrameInterleaver
  std::unique_ptr<AudioMixingConsole>  _audioMixer;
  
  // Animation Service Classes
  std::unique_ptr<AnimationSelector>          _animationSelector;
  std::unique_ptr<AnimationFrameInterleaver>  _frameInterleaver;
  std::unique_ptr<AnimationMessageBuffer>     _messageBuffer;
  
  
  // Container for all known "canned" animations (i.e. non-live)
  CannedAnimationContainer& _animationContainer;
  AnimationGroupContainer&  _animationGroups;
  
  // FIXME: TEST CODE
  void HandleMessage(const PlayAnimation_DEV& msg);
  
};
  
  
} // namespace RobotAnimation
} // namespace Cozmo
} // namespace Anki

#endif /* __Basestation_Animations_EngineAnimationController_H__ */
