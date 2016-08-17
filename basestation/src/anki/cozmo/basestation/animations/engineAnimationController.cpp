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

#include "anki/cozmo/basestation/cannedAnimationContainer.h"

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EngineAnimationController::EngineAnimationController(const CozmoContext* context, Audio::RobotAudioClient* audioClient)
: _context(context)
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
  
  IExternalInterface* interface = _context->GetExternalInterface();
  
  // Add Listners to Game to Engine events
  
  // Set DEV PlayAnimation action
  AddSignalHandle(interface->Subscribe(MessageGameToEngineTag::PlayAnimation_DEV,
                                       [this](const GameToEngineEvent& msg) {
                                         HandleMessage(msg.GetData().Get_PlayAnimation_DEV());
                                       }));
  
  // Set Robot output volume
  AddSignalHandle(interface->Subscribe(ExternalInterface::MessageGameToEngineTag::SetRobotVolume,
                                       [this] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& message) {
                                         _frameInterleaver->SetRobotMasterVolume(message.GetData().Get_SetRobotVolume().volume);
                                       }));
  
  
  
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
    
    // Buffer and store animtion
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


  
} // namespace RobotAnimation
} // namespcae Cozmo
} // namespace Anki
