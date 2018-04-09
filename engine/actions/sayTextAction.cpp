/**
 * File: sayTextAction.cpp
 *
 * Author: Andrew Stein
 * Date:   8/29/2014
 *
 * Description: Implements animation and audio cozmo-specific actions, derived from the IAction interface.
 *
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "engine/actions/sayTextAction.h"
#include "engine/animations/animationGroup/animationGroup.h"
#include "engine/animations/animationGroup/animationGroupContainer.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/cozmoContext.h"
#include "engine/robot.h"
#include "engine/robotInterface/messageHandler.h"

#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/common/engine/utils/timer.h"

#include "util/fileUtils/fileUtils.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"

using SayTextVoiceStyle = Anki::Cozmo::SayTextVoiceStyle;

#define LOG_CHANNEL "TextToSpeech"

#define DEBUG_SAYTEXT_ACTION 0

// Max duration of generated animation
//const float kMaxAnimationDuration_ms = 60000;  // 1 min

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

namespace Anki {
namespace Cozmo {

// Static members
SayTextAction::SayIntentConfigMap SayTextAction::_intentConfigs;

// Static Method
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SayTextAction::LoadMetadata(Util::Data::DataPlatform& dataPlatform)
{
  if (!_intentConfigs.empty()) {
    LOG_WARNING("SayTextAction.LoadMetadata.AttemptToReloadStaticData", "_intentConfigs");
    return false;
  }

  // Check for file
  static const std::string filePath = "config/engine/sayTextintentConfig.json";
  if (!Util::FileUtils::FileExists(dataPlatform.pathToResource(Util::Data::Scope::Resources, filePath))) {
    LOG_ERROR("SayTextAction.LoadMetadata.FileNotFound", "sayTextintentConfig.json");
    return false;
  }

  // Read file
  Json::Value json;
  if (!dataPlatform.readAsJson(Util::Data::Scope::Resources, filePath, json)) {
    LOG_ERROR("SayTextAction.LoadMetadata.CanNotRead", "sayTextintentConfig.json");
    return false;
  }

  // Load Intent Config
  if (json.isNull() || !json.isObject()) {
    LOG_ERROR("SayTextAction.LoadMetadata.json.IsNull", "or.NotIsObject");
    return false;
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
    DEV_ASSERT(intentEnumIt != sayTextIntentMap.end(), "SayTextAction.LoadMetadata.CanNotFindSayTextIntent");
    if (intentEnumIt != sayTextIntentMap.end()) {
      // Store Intent into STATIC var
      const SayTextIntentConfig config(name, *intentJsonIt, voiceStyleMap);
      _intentConfigs.emplace( intentEnumIt->second, std::move( config ) );
    }
  }

  return true;
}

// Public Methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::SayTextAction(const std::string& text,
                             const SayTextVoiceStyle style,
                             const float durationScalar,
                             const float pitchScalar)
: IAction("SayText",
          RobotActionType::SAY_TEXT,
          (u8)AnimTrackFlag::NO_TRACKS)
, _text(text)
, _style(style)
, _durationScalar(durationScalar)
, _pitchScalar(pitchScalar)
{

} // SayTextAction()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::SayTextAction(const std::string& text, const SayTextIntent intent)
: IAction("SayText",
          RobotActionType::SAY_TEXT,
          (u8)AnimTrackFlag::NO_TRACKS)
, _text(text)
, _intent(intent)
{

} // SayTextAction()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::~SayTextAction()
{
  // Cleanup TTS request, if any
  if (HasRobot()) {
    if (_ttsState == TextToSpeechState::Preparing
        || _ttsState == TextToSpeechState::Prepared
        || _ttsState == TextToSpeechState::Delivering) {
      LOG_DEBUG("SayTextAction.Destructor", "Cancel ttsID %d", _ttsID);
      RobotInterface::TextToSpeechCancel msg;
      msg.ttsID = _ttsID;
      const Robot & robot = GetRobot();
      robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
    }
  }
  // Clean up accompanying animation, if any
  if (_animAction) {
    _animAction->PrepForCompletion();
  }

} // ~SayTextAction()


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SayTextAction::OnRobotSet()
{
  // If constructor specifies intent, set style/duration/pitch to match
  if (_intent != SayTextIntent::Count) {
    const auto it = _intentConfigs.find(_intent);
    if (it != _intentConfigs.end()) {
      // Set intent values
      const SayTextIntentConfig& config = it->second;
      auto & robot = GetRobot();
      auto & rng = robot.GetRNG();

      // Set audio processing style type
      _style = config.style;

      // Get Duration val
      const auto & durationTrait = config.FindDurationTraitTextLength(Util::numeric_cast<uint>(_text.length()));
      _durationScalar = durationTrait.GetDuration(rng);

      // Get Pitch val
      const auto & pitchTrait = config.FindPitchTraitTextLength(Util::numeric_cast<uint>(_text.length()));
      _pitchScalar = pitchTrait.GetDuration(rng);
    } else {
      LOG_ERROR("SayTextAction.RobotSet.CanNotFind.SayTextIntentConfig", "%s", EnumToString(_intent));
    }
  }

  LOG_INFO("SayTextAction.RobotSet",
           "Text '%s' Intent '%s' Style '%s' DurScalar %f Pitch %f",
           Util::HidePersonallyIdentifiableInfo(_text.c_str()),
           EnumToString(_intent),
           EnumToString(_style),
           _durationScalar,
           _pitchScalar);

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SayTextAction::SetAnimationTrigger(AnimationTrigger trigger, u8 ignoreTracks)
{
  _animTrigger = trigger;
  _ignoreAnimTracks = ignoreTracks;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult SayTextAction::Init()
{
  using RobotToEngine = Anki::Cozmo::RobotInterface::RobotToEngine;
  using RobotToEngineTag = Anki::Cozmo::RobotInterface::RobotToEngineTag;
  using TextToSpeechPrepare = Anki::Cozmo::RobotInterface::TextToSpeechPrepare;

  // Assign a unique ID for this utterance. The ttsID is used to track lifetime
  // of data associated with each utterance.

  _ttsID = GetNextID();
  _ttsState = TextToSpeechState::Preparing;

  // When does this action expire?
  _expiration_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() + _timeout_sec;

  LOG_INFO("SayTextAction.Init", "ttsID %d text %s", _ttsID, Util::HidePersonallyIdentifiableInfo(_text.c_str()));

  // Set up a callback to process TTS events.  When we receive a terminal event,
  // the ttsState is updated to match.
  auto callback = [this](const AnkiEvent<RobotToEngine>& event)
  {
    const auto & ttsEvent = event.GetData().Get_textToSpeechEvent();
    const auto ttsID = ttsEvent.ttsID;

    // If this is our ID, update state to match
    if (ttsID == _ttsID) {
      LOG_DEBUG("SayTextAction.callback", "ttsID %hhu ttsState now %hhu", ttsID, ttsEvent.ttsState);
      _ttsState = ttsEvent.ttsState;
    }
  };

  // Subscribe to TTS events
  auto & robot = GetRobot();
  auto * messageHandler = robot.GetRobotMessageHandler();

  _signalHandle = messageHandler->Subscribe(RobotToEngineTag::textToSpeechEvent, callback);

  // Compose a request to prepare TTS audio
  TextToSpeechPrepare msg;
  msg.ttsID = _ttsID;
  msg.text = _text;
  msg.style = _style;
  msg.durationScalar = _durationScalar;
  msg.pitchScalar = _pitchScalar;

  // Send request to animation process
  const Result result = robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    LOG_ERROR("SayTextAction.Init", "Unable to send robot message (result %d)", result);
    _ttsState = TextToSpeechState::Invalid;
    return ActionResult::ABORT;
  }

  // Execution continues in CheckIfDone() below.
  // State is advanced in response to events from animation process.

  return ActionResult::SUCCESS;

} // Init()

ActionResult SayTextAction::TransitionToDelivering()
{
  LOG_DEBUG("SayTextAction::TransitionToDelivering", "ttsID %d is ready to deliver", _ttsID);

  if (!HasRobot()) {
    LOG_ERROR("SayTextAction.TransitionToDelivering.NoRobot", "ttsID %d has no robot", _ttsID);
    _ttsState = TextToSpeechState::Invalid;
    return ActionResult::ABORT;
  }

  _ttsState = TextToSpeechState::Delivering;

  RobotInterface::TextToSpeechDeliver msg;
  msg.ttsID = _ttsID;
  msg.triggeredByAnim = (_animTrigger != AnimationTrigger::Count);

  const auto & robot = GetRobot();
  const auto result = robot.SendMessage(RobotInterface::EngineToRobot(std::move(msg)));
  if (RESULT_OK != result) {
    LOG_ERROR("SayTextAction.TransitionToDelivering.SendMessage",
              "ttsID %d unable to send message (result %d)",
              _ttsID, result);
    return ActionResult::SEND_MESSAGE_TO_ROBOT_FAILED;
  }

  return ActionResult::RUNNING;
}

ActionResult SayTextAction::TransitionToPlaying()
{
  LOG_DEBUG("SayTextAction::TransitionToPlaying", "ttsID %d is ready to play", _ttsID);
  _ttsState = TextToSpeechState::Playing;
  _animAction = std::make_unique<TriggerAnimationAction>(_animTrigger, 1, false, _ignoreAnimTracks);
  _animAction->SetRobot(&GetRobot());
  return ActionResult::RUNNING;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ActionResult SayTextAction::CheckIfDone()
{

  // Has this action expired?
  const float now_sec = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  if (_expiration_sec < now_sec) {
    LOG_DEBUG("SayTextAction.CheckIfDone", "ttsID %d has expired", _ttsID);
    return ActionResult::TIMEOUT;
  }

  ActionResult result;
  switch (_ttsState) {
    case TextToSpeechState::Invalid:
    {
      // Something has gone wrong
      LOG_DEBUG("SayTextAction.CheckIfDone", "ttsID %d is invalid", _ttsID);
      result = ActionResult::CANCELLED_WHILE_RUNNING;
      break;
    }
    case TextToSpeechState::Preparing:
    {
      // Wait for state "prepared", nothing else to do
      result = ActionResult::RUNNING;
      break;
    }
    case TextToSpeechState::Prepared:
    {
      // Audio is ready, send it to wwise
      result = TransitionToDelivering();
      break;
    }
    case TextToSpeechState::Delivering:
    {
      // Wait for state "delivered", nothing else to do
      result = ActionResult::RUNNING;
      break;
    }
    case TextToSpeechState::Delivered:
    {
      // Audio has been sent to wwise, play animation (if any)
      if (_animTrigger != AnimationTrigger::Count) {
        result = TransitionToPlaying();
      } else {
        result = ActionResult::SUCCESS;
      }
      break;
    }
    case TextToSpeechState::Playing:
    {
      // Wait for animation to complete
      result = _animAction->Update();
      break;
    }
  }

  return result;
}



// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
#if 0
// VIC-2151: Fit-to-duration not supported on victor
void SayTextAction::UpdateAnimationToFitDuration(const float duration_ms)
{
  if (AnimationTrigger::Count != _animationTrigger) {
    // TODO: SayTextAction is broken (VIC-360)
    while (_animation.GetLastKeyFrameTime_ms() < duration_ms && duration_ms <= kMaxAnimationDuration_ms ) {
      const Animation* nextAnim = GetAnimation(_animationTrigger, _robot);
      if (nullptr != nextAnim) {
        _animation.AppendAnimation(*nextAnim);
      }
      else {
        PRINT_NAMED_ERROR("SayTextAction.UpdateAnimationToFitDuration.GetAnimationFailed",
                          "AnimationTrigger: %s", EnumToString(_animationTrigger));
        break;
      }
    }
  }
  else {
    PRINT_NAMED_WARNING("SayTextAction.UpdateAnimationToFitDuration.InvalidAnimationTrigger", "AnimationTrigger::Count");
  }
} // UpdateAnimationToFitDuration()
#endif

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// SayTextIntentConfig methods
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::SayTextIntentConfig::SayTextIntentConfig(const std::string& intentName,
                                                        const Json::Value& json,
                                                        const SayTextVoiceStyleMap& styleMap)
: name(intentName)
{
  // Set Voice Style
  const auto styleKey = json.get("style", Json::Value::null);
  if (!styleKey.isNull()) {
    const auto it = styleMap.find(styleKey.asString());
    DEV_ASSERT(it != styleMap.end(), "SayTextAction.LoadMetadata.IntentStyleNotFound");
    if (it != styleMap.end()) {
      style = it->second;
    }
  }

  // Duration Traits
  const auto durationTraitJson = json.get("durationTraits", Json::Value::null);
  if (!durationTraitJson.isNull()) {
    for (auto traitIt = durationTraitJson.begin(); traitIt != durationTraitJson.end(); ++traitIt) {
      durationTraits.emplace_back(*traitIt);
    }
  }

  // Pitch Traits
  const auto pitchTraitJson = json.get("pitchTraits", Json::Value::null);
  if (!pitchTraitJson.isNull()) {
    for (auto traitIt = pitchTraitJson.begin(); traitIt != pitchTraitJson.end(); ++traitIt) {
      pitchTraits.emplace_back(*traitIt);
    }
  }

  DEV_ASSERT(!name.empty(), "SayTextAction.LoadMetadata.Intent.name.IsEmpty");
  DEV_ASSERT(!durationTraitJson.empty(), "SayTextAction.LoadMetadata.Intent.durationTraits.IsEmpty");
  DEV_ASSERT(!pitchTraitJson.empty(), "SayTextAction.LoadMetadata.Intent.pitchTraits.IsEmpty");
} // SayTextIntentConfig()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SayTextAction::SayTextIntentConfig::ConfigTrait& SayTextAction::SayTextIntentConfig::FindDurationTraitTextLength(uint textLength) const
{
  for (const auto& aTrait : durationTraits) {
    if (aTrait.textLengthMin <= textLength && aTrait.textLengthMax >= textLength) {
      return aTrait;
    }
  }
  return durationTraits.front();
} // FindDurationTraitTextLength()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const SayTextAction::SayTextIntentConfig::ConfigTrait& SayTextAction::SayTextIntentConfig::FindPitchTraitTextLength(uint textLength) const
{
  for (const auto& aTrait : pitchTraits) {
    if (aTrait.textLengthMin <= textLength && aTrait.textLengthMax >= textLength) {
      return aTrait;
    }
  }
  return pitchTraits.front();
} // FindPitchTraitTextLength()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SayTextAction::SayTextIntentConfig::ConfigTrait::ConfigTrait(const Json::Value& json)
{
  textLengthMin = json.get("textLengthMin", Json::Value(std::numeric_limits<uint>::min())).asUInt();
  textLengthMax = json.get("textLengthMax", Json::Value(std::numeric_limits<uint>::max())).asUInt();
  rangeMin = json.get("rangeMin", Json::Value(std::numeric_limits<float>::min())).asFloat();
  rangeMax = json.get("rangeMax", Json::Value(std::numeric_limits<float>::max())).asFloat();
  rangeStepSize = json.get("stepSize", Json::Value(0.f)).asFloat(); // If No step size use Range Min and don't randomize
} // ConfigTrait()

float SayTextAction::SayTextIntentConfig::ConfigTrait::GetDuration(Util::RandomGenerator& randomGen) const
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
