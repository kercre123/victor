/*
 * File: engineAnimationController.cpp
 *
 * Author: Jordan Rivas
 * Created: 07/14/16
 *
 * Description:
 *
 * Copyright: Anki, Inc. 2016
 */


#include "anki/cozmo/basestation/animations/animationControllerTypes_Internal.h"

#include "anki/cozmo/basestation/animations/engineAnimationController.h"
#include "anki/cozmo/basestation/animations/animationFrameInterleaver.h"
#include "anki/cozmo/basestation/animations/animationMessageBuffer.h"
#include "anki/cozmo/basestation/animations/animationSelector.h"
#include "anki/cozmo/basestation/animations/streamingAnimation.h"


#include "anki/cozmo/basestation/audio/mixing/audioMixingConsole.h"
#include "anki/cozmo/basestation/audio/robotAudioClient.h"
//#include "anki/cozmo/basestation/audio/robotAudioOutputSource.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"

#include "anki/cozmo/basestation/animationContainers/cannedAnimationContainer.h"

#include "anki/cozmo/basestation/cozmoContext.h"
#include "anki/cozmo/basestation/robot.h"
#include "anki/cozmo/basestation/robotManager.h"



#include "anki/cozmo/basestation/events/ankiEvent.h"

#include "clad/types/animationKeyFrames.h"
#include "clad/externalInterface/messageGameToEngine.h"

#include <string>


namespace Anki {
namespace Cozmo {
namespace RobotAnimation {
  
#define AUDIO_FRAME_SAMPLE_COUNT  static_cast<size_t>(AnimConstants::AUDIO_SAMPLE_SIZE)

// Placeholder value for invalid tag
static constexpr IAnimationStreamer::Tag INVALID_TAG = 0;
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EngineAnimationController::EngineAnimationController(const CozmoContext* context, RobotAudioClient* audioClient) :
  IAnimationStreamer(context->GetExternalInterface())
, _context(context)
, _audioClient(audioClient)
, _audioMixer(new Audio::AudioMixingConsole(AUDIO_FRAME_SAMPLE_COUNT))
, _animationSelector(new AnimationSelector(*_audioClient))
, _frameInterleaver(new AnimationFrameInterleaver(*_audioMixer))
, _messageBuffer(new AnimationMessageBuffer())
, _animationContainer(_context->GetRobotManager()->GetCannedAnimations())
, _animationGroups(_context->GetRobotManager()->GetAnimationGroups())
{
  
  // Setup Messages
  using namespace ExternalInterface;
  using GameToEngineEvent = AnkiEvent<MessageGameToEngine>;
  
  // Add Listeners to Game to Engine events.
  // Note ExternalInterface is not present during unit tests.
  IExternalInterface* interface = _context->GetExternalInterface();
  if (interface != nullptr) {
    // Set DEV PlayAnimation action
    AddSignalHandle(interface->Subscribe(MessageGameToEngineTag::PlayAnimation_DEV,
                                         [this](const GameToEngineEvent& msg) {
                                           HandleMessage(msg.GetData().Get_PlayAnimation_DEV());
                                         }));
  
    // Set Robot output volume
    AddSignalHandle(interface->Subscribe(ExternalInterface::MessageGameToEngineTag::SetRobotVolume,
                                         [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& msg) {
                                           const float volume = msg.GetData().Get_SetRobotVolume().volume;
                                           _frameInterleaver->SetRobotMasterVolume(volume);
                                         }));
  }
  
  // Setup Modules
  // TODO: This feels like it should be temp code
  _frameInterleaver->SetAnimationCompletedCallback([this](const std::string& completedAnimation)
                                                   {
                                                     // TODO: Check meta if animation should stay in memory
                                                     _animationSelector->RemoveStreamingAnimation(completedAnimation);
                                                   });
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EngineAnimationController::~EngineAnimationController()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// If any animation is set for streaming and isn't done yet, stream it.
Result EngineAnimationController::Update(Robot& robot)
{
  // Update Robot Buffer with animations
  if (K_ANIMATION_CONTROLLER_UPDATE_TICK_LOG_LEVEL >= 3) {
    PRINT_CH_INFO(KAnimationControllerLogChannel, "AnimationController.Update", "Robot: %d", robot.GetID());
  }
  // Calculate Frames & Bytes for robot buffer
  // TODO: Check if there are animations playing before calculating?
  uint32_t framesToSend = AnimationMessageBuffer::CalculateNumberOfFramesToSend(robot);
  if (framesToSend > 0) {
    EngineToRobotMessageList msgList;
    
    // Attempt to buffer desired frame count
    for (size_t idx = 0; idx <= framesToSend; ++idx) {
      // Check if all dependancies are ready to tick next frame
      bool isReady = false;
      
      // TODO: Need to check if there is audio that should play without animation, aka background audio
      
      // TODO: Need way to not tick FrameInterleaver if there are no animations, ready animations & buffering animations
      
      const auto state = _frameInterleaver->Update();
      isReady = (state != AnimationFrameInterleaver::State::BufferingAnimation);
      
      
      if (!isReady) {
        // Break out of collecting animation data loop
        break;
      }
      
      // TODO: We are good to go on this frame need to prepare other dependencies (aka Background Sound)
      
      // Get Interleaved frames
      _frameInterleaver->PopFrameRobotMessages(msgList);
      
      // TODO: Need to check if there is audio that should play without animation, aka background audio

    }
    
    // Add interleaved frame messages to robot buffer
    _messageBuffer->BufferMessageToSend(msgList);
  }
  
  
  // TODO: Switcher can delete animation if done playing HERE
  // Maybe we just create a list of completed animations?
  
  
  // Send final animation frames messages to robot
  if (_messageBuffer->HasMessages()) {
    uint32_t bytesToSend = AnimationMessageBuffer::CalculateNumberOfBytesToSend(robot);
    _messageBuffer->SendMessages(robot, bytesToSend);
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// FIXME: Temp Code
void EngineAnimationController::HandleMessage(const ExternalInterface::PlayAnimation_DEV& msg)
{
  PRINT_CH_INFO(KAnimationControllerLogChannel,
                "AnimationController::HandleMessage", "Animation: '%s' Loop: %d RobotId: %d",
                msg.animationName.c_str(), msg.numLoops, msg.robotId);
  
  
  //FIXME: Remove test code
  Animation* anim = _animationContainer.GetAnimation(msg.animationName);
  if (anim != nullptr) {
    
    // Buffer and store animation
    _animationSelector->BufferStreamingAnimation(anim);
    StreamingAnimation* streamingAnimPtr = _animationSelector->GetStreamingAnimation(anim->GetName());
    
    // Play animation
    if (streamingAnimPtr != nullptr) {
      // Reset Animation tracks
      streamingAnimPtr->Init();
      _frameInterleaver->SetNextAnimation(streamingAnimPtr);
    }
  }
}

// Sets an animation to be streamed and how many times to stream it.
// Use numLoops = 0 to play the animation indefinitely.
// Returns a tag you can use to monitor whether the robot is done playing this
// animation.
// If interruptRunning == true, any currently-streaming animation will be aborted.
// Actual streaming occurs on calls to Update().
IAnimationStreamer::Tag EngineAnimationController::SetStreamingAnimation(Animation* anim, u32 numLoops, bool interruptRunning)
{
  DEV_ASSERT(false, "EngineAnimationController.SetStreamingAnimation.NotImplemented");
  return INVALID_TAG;
}
  
// Set the animation to be played when no other animation has been specified.  Use the empty string to
// disable idle animation. NOTE: this wipes out any idle animation stack (from the push/pop actions below)
Result EngineAnimationController::SetIdleAnimation(AnimationTrigger animName)
{
  DEV_ASSERT(false, "EngineAnimationController.SetIdleAnimation.NotImplemented");
  return RESULT_FAIL_INVALID_OBJECT;
}
  
// Set the idle animation and also add it to the idle animation stack, so we can use pop later. The current
// idle (even if it came from SetIdleAnimation) is always on the stack
Result EngineAnimationController::PushIdleAnimation(AnimationTrigger animName)
{
  DEV_ASSERT(false, "EngineAnimationController.PushIdleAnimation.NotImplemented");
  return RESULT_FAIL_INVALID_OBJECT;
}
  
// Return to the idle animation which was running prior to the most recent call to PushIdleAnimation.
// Returns RESULT_OK on success and RESULT_FAIL if the stack of idle animations was empty.
// Will not pop the last idle off the stack.
Result EngineAnimationController::PopIdleAnimation()
{
  DEV_ASSERT(false, "EngineAnimationController.PopIdleAnimation.NotImplemented");
  return RESULT_FAIL_INVALID_OBJECT;
}
  
// Add a procedural face "layer" to be combined with whatever is streaming
Result EngineAnimationController::AddFaceLayer(const std::string& name, FaceTrack&& faceTrack, TimeStamp_t delay_ms)
{
  DEV_ASSERT(false, "EngineAnimationController.AddFaceLayer.NotImplemented");
  return RESULT_FAIL_INVALID_OBJECT;
}
  
// Add a procedural face "layer" that is applied and then has its final
// adjustment "held" until removed.
// A handle/tag for the layer is returned, which is needed for removal.
IAnimationStreamer::Tag EngineAnimationController::AddPersistentFaceLayer(const std::string& name, FaceTrack&& faceTrack)
{
  DEV_ASSERT(false, "EngineAnimationController.AddPersistentFaceLayer.NotImplemented");
  return INVALID_TAG;
}
  
// Remove a previously-added persistent face layer using its tag.
// If duration > 0, that amount of time will be used to transition back
// to no adjustment
void EngineAnimationController::RemovePersistentFaceLayer(Tag tag, s32 duration_ms)
{
  DEV_ASSERT(false, "EngineAnimationController.RemovePersistentFaceLayer.NotImplemented");
}
  
// Add a keyframe to the end of an existing persistent face layer
void EngineAnimationController::AddToPersistentFaceLayer(Tag tag, ProceduralFaceKeyFrame&& keyframe)
{
  DEV_ASSERT(false, "EngineAnimationController.AddToPersistentFaceLayer.NotImplemented");
}
  
// Remove any existing procedural eye dart created by KeepFaceAlive
void EngineAnimationController::RemoveKeepAliveEyeDart(s32 duration_ms)
{
  DEV_ASSERT(false, "EngineAnimationController.RemoveKeepAliveEyeDart.NotImplemented");
}
  
const Animation* EngineAnimationController::GetStreamingAnimation() const
{
  DEV_ASSERT(false, "EngineAnimationController.GetStreamingAnimation.NotImplemented");
  return nullptr;
}
  
const std::string& EngineAnimationController::GetAnimationNameFromGroup(const std::string& name, const Robot& robot) const
{
  DEV_ASSERT(false, "EngineAnimationController.GetAnimationNameFromGroup.NotImplemented");
  static const std::string invalid("invalid");
  return invalid;
}
  
void EngineAnimationController::SetDefaultParams()
{
  DEV_ASSERT(false, "EngineAnimationController.SetDefaultParams.NotImplemented");
}
  
} // namespace RobotAnimation
} // namespace Cozmo
} // namespace Anki
