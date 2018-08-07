/**
* File: textToSpeechCoordinator.cpp
*
* Author: Sam Russell
* Created: 5/22/18
*
* Description: Component for requesting and tracking TTS utterance generation and playing
*
* Copyright: Anki, Inc. 2018
*
**/

#include "components/textToSpeech/textToSpeechCoordinator.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/robotComponents_fwd.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/math/math.h"

#include <cstring>

// Return a serial number 1-255.
// 0 is reserved for "invalid".
static uint8_t GetNextID()
{
  static uint8_t ttsID = 0;
  uint8_t id = ++ttsID;
  if (id == 0) {
    id = ++ttsID;
  }
  return id;
}

namespace Anki{
namespace Vector{

namespace {
  // The coordinator will wait kPlayTimeoutScalar times the expected duration before erroring out on a TTS utterance
  constexpr float kPlayTimeoutScalar = 2.0f;

  // Utterances should not take longer than this to generate
  constexpr float kGenerationTimeout_s = 20.0f; // making this high until we figure out accurate generation times
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechCoordinator::TextToSpeechCoordinator()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::TextToSpeechCoordinator)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechCoordinator::InitDependent(Vector::Robot* robot, const RobotCompMap& dependentComps)
{
  // Keep a pointer to robot for message sending
  _robot = robot;

  // Set up TextToSpeech status message handling
  auto callback = [this](const AnkiEvent<RobotInterface::RobotToEngine>& event)
  {
    const auto& ttsEvent = event.GetData().Get_textToSpeechEvent();
    UpdateUtteranceState(ttsEvent.ttsID, ttsEvent.ttsState, ttsEvent.expectedDuration_ms);
  };

  auto* messageHandler = _robot->GetRobotMessageHandler();
  _signalHandle = messageHandler->Subscribe(RobotInterface::RobotToEngineTag::textToSpeechEvent, 
                                            callback);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechCoordinator::UpdateDependent(const RobotCompMap& dependentComps)
{
  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  size_t currentTick = BaseStationTimer::getInstance()->GetTickCount();

  // Check for timeouts
  for(auto it =  _utteranceMap.cbegin(); it != _utteranceMap.cend(); ){
    const auto& utteranceID = it->first;
    const auto& utterance = it->second;
    // Generation Timeout
    if(UtteranceState::Generating == utterance.state &&
       (currentTime_s - utterance.timeStartedGeneration_s) > kGenerationTimeout_s){
      PRINT_NAMED_ERROR("TextToSpeechCoordinator.Update.GenerationTimeout",
                        "Utterance %d generation timed out after %f seconds",
                        utteranceID,
                        kGenerationTimeout_s);
      // Remove the utterance so status update requests will return invalid
      UpdateUtteranceState(utteranceID, TextToSpeechState::Invalid);
    }
    // Playing timeout
    if(UtteranceState::Playing == utterance.state){
      float playTime_s = currentTime_s - utterance.timeStartedPlaying_s;
      float playingTimeout_s = utterance.expectedDuration_s * kPlayTimeoutScalar;
      if(playTime_s >= playingTimeout_s){
        PRINT_NAMED_WARNING("TextToSpeechCoordinator.Update.UtterancePlayingTimeout",
                            "Utterance %d was expected to play for %f seconds, timed out after %f",
                            utteranceID,
                            utterance.expectedDuration_s,
                            playTime_s);
        UpdateUtteranceState(utteranceID, TextToSpeechState::Invalid);
      }
    }
    // Clean up finished utterances after one tick  
    if( ((UtteranceState::Finished == utterance.state) ||
        (UtteranceState::Invalid == utterance.state)) &&
        (currentTick > utterance.tickReadyForCleanup) ){
      PRINT_NAMED_DEBUG("TextToSpeechCoordinator.Update.CleanUpUtterance",
                        "Utterance %d cleaned one BSTick after it finished playing",
                        utteranceID);
      it = _utteranceMap.erase(it);
    } else {
      ++it;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Public Interface methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t TextToSpeechCoordinator::CreateUtterance(const std::string& utteranceString,
                                                       const UtteranceTriggerType& triggerType,
                                                       const AudioTtsProcessingStyle& style,
                                                       const float durationScalar,
                                                       const UtteranceUpdatedCallback callback)
{
  uint8_t utteranceID = GetNextID();

  // TODO:(str) we will eventually need to support anim keyframe driven tts
  // Remove when the TTSCoordinator can handle that case
  ANKI_VERIFY(UtteranceTriggerType::KeyFrame != triggerType,
              "TextToSpeechCoordinator.KeyframeDrivenTTS.NotYetSupported",
              "Keyframe driven TTS should not be used with the TTSCoordinator until it is fully supported");

  RobotInterface::TextToSpeechPrepare msg;
  const size_t maxTextLength = msg.text.max_size() - 1;

  // we store the string in a 1024 character array, so we need can't have a string greater than this (including the
  // null terminating character).
  if(utteranceString.length() > maxTextLength){
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.CreateUtterance",
                        "Utterance string cannot be longer than %d characters (yours is %d)",
                        (int)maxTextLength,
                        (int)utteranceString.length());
    return kInvalidUtteranceID;
  }

  // Compose a request to prepare TTS audio
  msg.ttsID = utteranceID;
  msg.style = style;
  msg.durationScalar = durationScalar;
  // copy our null-terminated string
  std::memcpy( msg.text.data(), utteranceString.c_str(), utteranceString.size() + 1 );

  // Send request to animation process
  const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.CreateUtterance", "Unable to send robot message (result %d)", result);
    return kInvalidUtteranceID;
  }

  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  UtteranceRecord newRecord;
  newRecord.state = UtteranceState::Invalid;
  newRecord.triggerType = triggerType;
  newRecord.timeStartedGeneration_s = currentTime_s;
  newRecord.callback = callback;

  _utteranceMap[utteranceID] = newRecord;
  // Update the state here so that the callback is run at least once before the user checks the state
  // of the utterance they've just created
  UpdateUtteranceState(utteranceID, TextToSpeechState::Preparing);

  PRINT_NAMED_INFO("TextToSpeechCoordinator.CreateUtterance",
                   "Text '%s' Style '%s' DurScalar %f",
                   Util::HidePersonallyIdentifiableInfo(utteranceString.c_str()),
                   EnumToString(style),
                   durationScalar);

  return utteranceID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const UtteranceState TextToSpeechCoordinator::GetUtteranceState(const uint8_t utteranceID) const
{
  auto it = _utteranceMap.find(utteranceID);
  if(it != _utteranceMap.end()){
    return it->second.state;
  } else {
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.UnknownUtteranceID",
                      "State was requested for utterance %d for which there is no valid record",
                      utteranceID);
    return UtteranceState::Invalid;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool TextToSpeechCoordinator::PlayUtterance(const uint8_t utteranceID)
{
  auto it = _utteranceMap.find(utteranceID);
  if(it == _utteranceMap.end()){
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.PlayUtterance.UnknownUtterance",
                      "Requested to play utterance %d for which there is no valid record",
                      utteranceID);
    return false;
  }

  const auto& utterance = it->second;
  if(UtteranceState::Invalid == utterance.state){
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.PlayUtterance.Invalid",
                      "Requested to play utterance %d which is marked invalid",
                      utteranceID);
    return false;
  }

  if(UtteranceState::Ready != utterance.state){
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.PlayUtterance.NotReady",
                      "Requested to play utterance %d which is not ready to play",
                      utteranceID);
    return false;
  }

  if(UtteranceTriggerType::Manual != utterance.triggerType){
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.PlayUtterance.NotManuallyPlayable",
                      "Requested to play utterance %d, which is not manually playable",
                      utteranceID);
    return false;
  }

  RobotInterface::TextToSpeechPlay msg;
  msg.ttsID = utteranceID;
  const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.PlayUtterance.FailedToSend",
                      "Unable to send robot message (result %d)", result);
    return false;
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const bool TextToSpeechCoordinator::CancelUtterance(const uint8_t utteranceID)
{
  auto it = _utteranceMap.find(utteranceID);
  if(it == _utteranceMap.end()){
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.UnknownUtteranceID",
                      "Requested to cancel utterance %d for which there is no valid record",
                      utteranceID);
    return false;
  }

  // we can cancel the tts at any point during it's lifecycle
  // notes:
  //  + if cancelling when it's generating, thread will hang until generation is complete
  //  + cancelling will clear the wav data in AnkiPluginInterface, regardless of which utteranceID was last delivered
  const UtteranceState utteranceState = it->second.state;
  if((UtteranceState::Invalid != utteranceState) && (UtteranceState::Finished != utteranceState))
  {
    PRINT_NAMED_INFO("TextToSpeechCoordinator.CancelUtteranceGeneration", "Cancel ttsID %d", utteranceID);
    RobotInterface::TextToSpeechCancel msg;
    msg.ttsID = utteranceID;
    const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
    if (RESULT_OK != result) {
      PRINT_NAMED_ERROR("TextToSpeechCoordinator.CancelUtteranceGeneration",
                        "Unable to send robot message (result %d)",
                        result);
    }

    // Mark the record for cleanup
    UpdateUtteranceState(utteranceID, TextToSpeechState::Invalid);
  }

  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Private methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechCoordinator::UpdateUtteranceState(const uint8_t& ttsID,
                                                   const TextToSpeechState& ttsState,
                                                   const float& expectedDuration_ms)
{
  auto it = _utteranceMap.find(ttsID);
  if(it == _utteranceMap.end()){
    PRINT_NAMED_WARNING("TextToSpeechCoordinator.StateUpdate.Invalid",
                        "Received state update for invalid utterance %d",
                        ttsID);
    return;
  }

  UtteranceRecord& utterance = it->second;
  float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  size_t currentTick = BaseStationTimer::getInstance()->GetTickCount();

  switch(ttsState){
    case TextToSpeechState::Invalid:
    {
      PRINT_NAMED_WARNING("TextToSpeechCoordinator.StateUpdate", "ttsID %d operation failed", ttsID);
      utterance.state = UtteranceState::Invalid;
      break;
    }
    case TextToSpeechState::Preparing:
    {
      PRINT_NAMED_INFO("TextToSpeechCoordinator.StateUpdate", "ttsID %d is being prepared", ttsID);
      utterance.state = UtteranceState::Generating;
      break;
    }
    case TextToSpeechState::Prepared:
    {
      PRINT_NAMED_INFO("TextToSpeechCoordinator.StateUpdate", "ttsID %d is ready to deliver", ttsID);

      RobotInterface::TextToSpeechDeliver msg;
      msg.ttsID = ttsID;
      msg.playImmediately = (UtteranceTriggerType::Immediate == utterance.triggerType);

      const auto result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
      if (RESULT_OK != result) {
        PRINT_NAMED_ERROR("TextToSpeechCoordinator.TransitionToDelivering.SendMessage",
                          "ttsID %d unable to send message (result %d)",
                          ttsID, result);
        utterance.state = UtteranceState::Invalid;
      } else {
        utterance.state = UtteranceState::Generating;
      }
      break;
    }
    case TextToSpeechState::Delivering:
    {
      PRINT_NAMED_INFO("TextToSpeechCoordinator.StateUpdate", "ttsID %d is being delivered", ttsID);
      utterance.state = UtteranceState::Generating;
      break;
    }
    case TextToSpeechState::Delivered:
    {
      PRINT_NAMED_INFO("TextToSpeechCoordinator.StateUpdate", "ttsID %d is ready to play", ttsID);
      utterance.state = UtteranceState::Ready;
      utterance.expectedDuration_s = Util::MilliSecToSec(expectedDuration_ms);
      break;
    }
    case TextToSpeechState::Playing:
    {
      PRINT_NAMED_INFO("TextToSpeechCoordinator.StateUpdate", "ttsID %d is now playing", ttsID);
      if(UtteranceTriggerType::KeyFrame == utterance.triggerType){
        // Keyframe driven utterances will never transition to finished, mark invalid since future
        // data and commands are not usable.
        utterance.state = UtteranceState::Invalid;
      } else{
        utterance.state = UtteranceState::Playing;
        utterance.timeStartedPlaying_s = currentTime_s;
      }
      break;
    }
    case TextToSpeechState::Finished:
    {
      float timeSinceStartedPlaying_s = currentTime_s - utterance.timeStartedPlaying_s;
      PRINT_NAMED_INFO("TextToSpeechCoordinator.StateUpdate.FinishedPlaying",
                       "ttsID %d has finished playing after %f seconds",
                       ttsID,
                       timeSinceStartedPlaying_s);
      utterance.state = UtteranceState::Finished;
      utterance.tickReadyForCleanup = currentTick; 
      break;
    }
  }

  // Notify the originator of the state change if they provided a callback
  if(nullptr != utterance.callback){
    utterance.callback(utterance.state);
  }
}

} // namespace Vector
} // namespace Anki


