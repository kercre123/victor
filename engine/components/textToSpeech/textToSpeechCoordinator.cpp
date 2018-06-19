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

#include "textToSpeechCoordinator.h"

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/components/dataAccessorComponent.h"
#include "engine/robotComponents_fwd.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/types/textToSpeechTypes.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#include "util/entityComponent/dependencyManagedEntity.h"
#include "util/math/math.h"

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
namespace Cozmo{

namespace {
  // The coordinator will wait kPlayTimeoutScalar times the expected duration before erroring out on a TTS utterance
  const float kPlayTimeoutScalar = 2.0f;
}
// Static members
TextToSpeechCoordinator::SayIntentConfigMap TextToSpeechCoordinator::_intentConfigs;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechCoordinator::TextToSpeechCoordinator()
: IDependencyManagedComponent<RobotComponentID>(this, RobotComponentID::TextToSpeechCoordinator)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TextToSpeechCoordinator::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
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

  if (!_intentConfigs.empty()) {
    PRINT_NAMED_WARNING("TextToSpeechCoordinator.LoadMetadata.AttemptToReloadStaticData", "_intentConfigs");
    return;
  }

  DataAccessorComponent* dataAccessor = dependentComps.GetComponentPtr<DataAccessorComponent>();
  const Json::Value& json = *(dataAccessor->GetTextToSpeechConfig());

  // Load Intent Config
  if (json.isNull() || !json.isObject()) {
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.LoadMetadata.json.IsNull", "or.NotIsObject");
    return;
  }

  // Create Cozmo Says Voice Style map
  SayTextVoiceStyleMap voiceStyleMap;
  for (uint8_t aStyleIdx = 0; aStyleIdx <  Util::numeric_cast<uint8_t>(SayTextVoiceStyle::Count); ++aStyleIdx) {
    const SayTextVoiceStyle aStyle = static_cast<SayTextVoiceStyle>(aStyleIdx);
    voiceStyleMap.emplace( EnumToString(aStyle), aStyle );

  }

  // Create Say Text Intent Map
  std::unordered_map<std::string, SayTextIntent> sayTextIntentMap;
  for (uint8_t anIntentIdx = 0; anIntentIdx < SayTextIntentNumEntries; ++anIntentIdx) {
    const SayTextIntent anIntent = static_cast<SayTextIntent>(anIntentIdx);
    sayTextIntentMap.emplace( EnumToString(anIntent), anIntent );
  }

  // Store metadata's Intent objects
  for (auto intentJsonIt = json.begin(); intentJsonIt != json.end(); ++intentJsonIt) {
    const std::string& name = intentJsonIt.key().asString();
    const auto intentEnumIt = sayTextIntentMap.find( name );
    DEV_ASSERT(intentEnumIt != sayTextIntentMap.end(),
               "TextToSpeechCoordinator.LoadMetadata.CanNotFindSayTextIntent");
    if (intentEnumIt != sayTextIntentMap.end()) {
      // Store Intent into STATIC var
      const SayTextIntentConfig config(name, *intentJsonIt, voiceStyleMap);
      _intentConfigs.emplace( intentEnumIt->second, std::move( config ) );
    }
  }
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
                                                       const SayTextVoiceStyle& style,
                                                       const UtteranceTriggerType& triggerType,
                                                       const float durationScalar,
                                                       const float pitchScalar,
                                                       const UtteranceUpdatedCallback callback)
{
  uint8_t utteranceID = GetNextID();

  // TODO:(str) we will eventually need to support anim keyframe driven tts
  // Remove when the TTSCoordinator can handle that case
  ANKI_VERIFY(UtteranceTriggerType::KeyFrame != triggerType,
              "TextToSpeechCoordinator.KeyframeDrivenTTS.NotYetSupported",
              "Keyframe driven TTS should not be used with the TTSCoordinator until it is fully supported");

  // Compose a request to prepare TTS audio
  RobotInterface::TextToSpeechPrepare msg;
  msg.ttsID = utteranceID;
  msg.text = utteranceString;
  msg.style = style;
  msg.durationScalar = durationScalar;
  msg.pitchScalar = pitchScalar;

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
                   "Text '%s' Style '%s' DurScalar %f Pitch %f",
                   Util::HidePersonallyIdentifiableInfo(utteranceString.c_str()),
                   EnumToString(style),
                   durationScalar,
                   pitchScalar);

  return utteranceID;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const uint8_t TextToSpeechCoordinator::CreateUtterance(const std::string& utteranceString, 
                                                       const SayTextIntent& intent,
                                                       const UtteranceTriggerType& triggerType,
                                                       const UtteranceUpdatedCallback callback)
{
  // Set style/duration/pitch from intent
  if (ANKI_VERIFY((intent != SayTextIntent::Count),
                  "TextToSpeechCoordinator.CreateUtterance.Failed",
                  "SayTextIntent::Count is not a valid intent")) {
    const auto it = _intentConfigs.find(intent);
    if (it != _intentConfigs.end()) {
      // Set intent values
      const SayTextIntentConfig& config = it->second;
      auto & rng = _robot->GetRNG();

      // Set audio processing style type
      SayTextVoiceStyle style = config.style;

      // Get Duration val
      const auto & durationTrait = config.FindDurationTraitTextLength(Util::numeric_cast<uint>(utteranceString.length()));
      float durationScalar = durationTrait.GetDuration(rng);

      // Get Pitch val
      const auto & pitchTrait = config.FindPitchTraitTextLength(Util::numeric_cast<uint>(utteranceString.length()));
      float pitchScalar = pitchTrait.GetDuration(rng);

      PRINT_NAMED_INFO("TextToSpeechCoordinator.CreateUtterance.UsingIntent",
                       "Intent '%s'",
                       EnumToString(intent));

      return CreateUtterance(utteranceString, style, triggerType, durationScalar, pitchScalar, callback);
    } else {
      PRINT_NAMED_ERROR("TextToSpeechCoordinator.CreateUtterance.CanNotFind.SayTextIntentConfig",
                        "%s",
                        EnumToString(intent));
    }
  } 

  return kInvalidUtteranceID;
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

  auto& utterance = it->second;

  // If the utterance is already playing, it cannot be cancelled
  if (UtteranceState::Playing == utterance.state){
    PRINT_NAMED_ERROR("TextToSpeechCoordinator.CancelUtteranceError",
                      "Cannot cancel utterance %d as it is already playing",
                      utteranceID);
    return false;
  }

  // If the utterance is still in generation, cancel generation 
  if (UtteranceState::Generating == utterance.state) {
    PRINT_NAMED_INFO("TextToSpeechCoordinator.CancelUtteranceGeneration", "Cancel ttsID %d", utteranceID);
    RobotInterface::TextToSpeechCancel msg;
    msg.ttsID = utteranceID;
    const Result result = _robot->SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
    if (RESULT_OK != result) {
      PRINT_NAMED_ERROR("TextToSpeechCoordinator.CancelUtteranceGeneration",
                        "Unable to send robot message (result %d)",
                        result);
    }
  }

  // Mark the record for cleanup
  UpdateUtteranceState(utteranceID, TextToSpeechState::Invalid);

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SayTextIntentConfig methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace{
const char* kStyleKey         = "style";
const char* kDurationTraitKey = "durationTraits";
const char* kPitchTraitKey    = "pitchTraits";
}

TextToSpeechCoordinator::SayTextIntentConfig::SayTextIntentConfig(const std::string& intentName,
                                                                  const Json::Value& json,
                                                                  const SayTextVoiceStyleMap& styleMap)
: name(intentName)
{
  // Set Voice Style
  const auto styleKey = json.get(kStyleKey, Json::Value::null);
  if (!styleKey.isNull()) {
    const auto it = styleMap.find(styleKey.asString());
    DEV_ASSERT(it != styleMap.end(), "TextToSpeechCoordinator.LoadMetadata.IntentStyleNotFound");
    if (it != styleMap.end()) {
      style = it->second;
    }
  }

  // Duration Traits
  const auto durationTraitJson = json.get(kDurationTraitKey, Json::Value::null);
  if (!durationTraitJson.isNull()) {
    for (auto traitIt = durationTraitJson.begin(); traitIt != durationTraitJson.end(); ++traitIt) {
      durationTraits.emplace_back(*traitIt);
    }
  }

  // Pitch Traits
  const auto pitchTraitJson = json.get(kPitchTraitKey, Json::Value::null);
  if (!pitchTraitJson.isNull()) {
    for (auto traitIt = pitchTraitJson.begin(); traitIt != pitchTraitJson.end(); ++traitIt) {
      pitchTraits.emplace_back(*traitIt);
    }
  }

  DEV_ASSERT(!name.empty(), "TextToSpeechCoordinator.LoadMetadata.Intent.name.IsEmpty");
  DEV_ASSERT(!durationTraitJson.empty(), "TextToSpeechCoordinator.LoadMetadata.Intent.durationTraits.IsEmpty");
  DEV_ASSERT(!pitchTraitJson.empty(), "TextToSpeechCoordinator.LoadMetadata.Intent.pitchTraits.IsEmpty");
} // SayTextIntentConfig()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const TextToSpeechCoordinator::SayTextIntentConfig::ConfigTrait& TextToSpeechCoordinator::SayTextIntentConfig::FindDurationTraitTextLength(uint textLength) const
{
  for (const auto& aTrait : durationTraits) {
    if (aTrait.textLengthMin <= textLength && aTrait.textLengthMax >= textLength) {
      return aTrait;
    }
  }
  return durationTraits.front();
} // FindDurationTraitTextLength()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const TextToSpeechCoordinator::SayTextIntentConfig::ConfigTrait& TextToSpeechCoordinator::SayTextIntentConfig::FindPitchTraitTextLength(uint textLength) const
{
  for (const auto& aTrait : pitchTraits) {
    if (aTrait.textLengthMin <= textLength && aTrait.textLengthMax >= textLength) {
      return aTrait;
    }
  }
  return pitchTraits.front();
} // FindPitchTraitTextLength()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TextToSpeechCoordinator::SayTextIntentConfig::ConfigTrait::ConfigTrait(const Json::Value& json)
{
  textLengthMin = json.get("textLengthMin", Json::Value(std::numeric_limits<uint>::min())).asUInt();
  textLengthMax = json.get("textLengthMax", Json::Value(std::numeric_limits<uint>::max())).asUInt();
  rangeMin = json.get("rangeMin", Json::Value(std::numeric_limits<float>::min())).asFloat();
  rangeMax = json.get("rangeMax", Json::Value(std::numeric_limits<float>::max())).asFloat();
  rangeStepSize = json.get("stepSize", Json::Value(0.f)).asFloat(); // If No step size use Range Min and don't randomize
} // ConfigTrait()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
float TextToSpeechCoordinator::SayTextIntentConfig::ConfigTrait::GetDuration(Util::RandomGenerator& randomGen) const
{
  // TODO: Move this into Random Util class
  float resultVal;
  if (Util::IsFltGTZero( rangeStepSize )) {
    // (Scalar Range / stepSize) + 1 = number of total possible steps
    const int stepCount = ((rangeMax - rangeMin) / rangeStepSize) + 1;
    const auto randStep = randomGen.RandInt( stepCount );
    resultVal = rangeMin + (rangeStepSize * randStep);
  }
  else {
    resultVal = rangeMin;
  }
  return resultVal;
} // GetDuration()

} // namespace Cozmo
} // namespace Anki


