/**
 * File: moodManager
 *
 * Author: Mark Wesley
 * Created: 10/14/15
 *
 * Description: Manages the Mood (a selection of emotions) for a Cozmo Robot
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "engine/moodSystem/moodManager.h"

#include "clad/audio/audioParameterTypes.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/vizInterface/messageViz.h"
#include "coretech/common/engine/utils/timer.h"
#include "engine/actions/actionContainers.h"
#include "engine/actions/actionInterface.h"
#include "engine/ankiEventUtil.h"
#include "engine/audio/engineRobotAudioClient.h"
#include "engine/components/robotStatsTracker.h"
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/externalInterface/externalMessageRouter.h"
#include "engine/externalInterface/gatewayInterface.h"
#include "engine/moodSystem/emotionEvent.h"
#include "engine/moodSystem/staticMoodData.h"
#include "engine/robot.h"
#include "engine/utils/cozmoFeatureGate.h"
#include "engine/viz/vizManager.h"
#include "proto/external_interface/messages.pb.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "webServerProcess/src/webService.h"
#include <assert.h>

namespace Anki {
namespace Cozmo {

namespace{
  const std::string kWebVizModuleName = "mood";
}

// For now StaticMoodData is basically a singleton, but hidden behind an interface in mood manager incase we ever
// need it to be different per robot / moodManager
static StaticMoodData sStaticMoodData;

CONSOLE_VAR_EXTERN(float, kTimeMultiplier);

namespace {
static const char* kActionResultEmotionEventKey = "actionResultEmotionEvents";
static const char* kAudioParametersMapKey = "audioParameterMap";
static const char* kSimpleMoodAudioKey = "simpleMoodAudioParameters";

CONSOLE_VAR(float, kMoodManager_AudioSendPeriod_s, "MoodManager", 0.5f);
CONSOLE_VAR(float, kMoodManager_WebVizPeriod_s, "MoodManager", 1.0f);
CONSOLE_VAR(float, kMoodManager_AppPeriod_s, "MoodManager", 1.0f);
}


StaticMoodData& MoodManager::GetStaticMoodData()
{
  return sStaticMoodData;
}


float MoodManager::GetCurrentTimeInSeconds()
{
  const float currentTimeInSeconds = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  return currentTimeInSeconds;
}


MoodManager::MoodManager()
: IDependencyManagedComponent(this, RobotComponentID::MoodManager)
, _lastUpdateTime(0.0f)
, _fixedEmotions{}
, _simpleMoodAudioParameter(AudioParameterType::Invalid)
{
  _pendingAppEvents.reserve(3); // ~max expected events in a single Update() tick
}

MoodManager::~MoodManager()
{
  // if the robot is destructing, it might not have an action list, so check that here
  if( _actionCallbackID != 0 && _robot != nullptr && _robot->HasComponent<ActionList>() ) {
    _robot->GetActionList().UnregisterCallback(_actionCallbackID);
    _actionCallbackID = 0;
  }
}

void MoodManager::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComps)
{
  _robot = robot;
  auto& context = dependentComps.GetComponent<ContextWrapper>().context;

  if (nullptr != context->GetDataPlatform())
  {
    ReadMoodConfig(context->GetDataLoader()->GetRobotMoodConfig());
    LoadEmotionEvents(context->GetDataLoader()->GetEmotionEventJsons());
  }

  if( ANKI_DEV_CHEATS ) {
    SubscribeToWebViz();
  }
}


void MoodManager::ReadMoodConfig(const Json::Value& inJson)
{
  GetStaticMoodData().Init(inJson);

  LoadAudioParameterMap(inJson[kAudioParametersMapKey]);
  LoadAudioSimpleMoodMap(inJson[kSimpleMoodAudioKey]);
  VerifyAudioEvents();
    
  LoadActionCompletedEventMap(inJson[kActionResultEmotionEventKey]);

  // set values per mood if we have them
  for( const auto& valueRangeEntry : GetStaticMoodData().GetEmotionValueRangeMap() ) {
    const auto& emotionType = valueRangeEntry.first;
    const auto& range = valueRangeEntry.second;

    GetEmotion(emotionType).SetEmotionValueRange(range.first, range.second);
  }

  if (nullptr != _robot) {
    if( _robot->HasExternalInterface() ) {
      auto helper = MakeAnkiEventUtil(*_robot->GetExternalInterface(), *this, _signalHandles);
      using namespace ExternalInterface;
      helper.SubscribeGameToEngine<MessageGameToEngineTag::MoodMessage>();
    }

    _actionCallbackID = _robot->GetActionList().RegisterActionEndedCallbackForAllActions(
      std::bind(&MoodManager::HandleActionEnded, this, std::placeholders::_1));
  }
}

void MoodManager::VerifyAudioEvents() const
{
  std::set<AudioParameterType> audioParams;

  for( const auto& mapPair : _audioParameterMap ) {
    const auto result = audioParams.insert(mapPair.second);
    ANKI_VERIFY(result.second,
                "MoodManager.VerifyAudioEvents.AudioMap.DuplicateEvent",
                "config '%s' specifies multiple emotions using the '%s' event",
                kAudioParametersMapKey,
                EnumToString(mapPair.second));
  }

  if( _simpleMoodAudioParameter != AudioParameterType::Invalid ) {
    const auto result = audioParams.insert(_simpleMoodAudioParameter);
    ANKI_VERIFY(result.second,
                "MoodManager.VerifyAudioEvents.AudioSimpleMap.DuplicateEvent",
                "config '%s' specifies the same event '%s' as is used in the emotion map",
                kSimpleMoodAudioKey,
                EnumToString(_simpleMoodAudioParameter));
  }
}


void MoodManager::LoadEmotionEvents(const RobotDataLoader::FileJsonMap& emotionEventData)
{
  for (const auto& fileJsonPair : emotionEventData)
  {
    const auto& filename = fileJsonPair.first;
    const auto& eventJson = fileJsonPair.second;
    if (!eventJson.empty() && LoadEmotionEvents(eventJson))
    {
      //PRINT_NAMED_DEBUG("MoodManager.LoadEmotionEvents", "Loaded '%s'", filename.c_str());
    }
    else
    {
      PRINT_NAMED_WARNING("MoodManager.LoadEmotionEvents", "Failed to read '%s'", filename.c_str());
    }
  }
}

bool MoodManager::LoadEmotionEvents(const Json::Value& inJson)
{
  return GetStaticMoodData().LoadEmotionEvents(inJson);
}

void MoodManager::LoadAudioParameterMap(const Json::Value& inJson)
{
  if( inJson.isNull() ) {
    return;
  }
  
  if( ANKI_VERIFY( ! inJson.isArray(), "MoodManager.LoadAudioParameterMap.MissingKey",
                   "No audio parameter map specified, or it isn't a list" ) ) {

    for( auto mapIt = inJson.begin(); mapIt != inJson.end(); ++mapIt ) {
      EmotionType emo;
      if( ANKI_VERIFY( EmotionTypeFromString(mapIt.key().asCString(), emo),
                       "MoodManager.LoadAudioParameterMap.InvalidEmotion",
                       "Emotion type '%s' cannot be converted to enum value",
                       mapIt.key().asCString() ) ) {

        AudioParameterType audioParam;
        if( ANKI_VERIFY( AudioMetaData::GameParameter::ParameterTypeFromString( mapIt->asCString(), audioParam ),
                       "MoodManager.LoadAudioParameterMap.InvalidAudioParameter",
                         "Audio parameter type '%s' cannot be converted to enum value",
                         mapIt->asCString() ) ) {
          _audioParameterMap.emplace( emo, audioParam );
        }
      }
    }
  }
}

void MoodManager::LoadAudioSimpleMoodMap(const Json::Value& inJson)
{
  if( inJson.isNull() ) {
    return;
  }

  const std::string& audioParamStr = JsonTools::ParseString(inJson,
                                                            "event",
                                                            "MoodManager.AudioSimpleMoodMap.ConfigError");
  
  ANKI_VERIFY( AudioMetaData::GameParameter::ParameterTypeFromString( audioParamStr, _simpleMoodAudioParameter ),
               "MoodManager.LoadAudioSimpleMoodMap.InvalidAudioParameter",
               "Audio parameter type '%s' cannot be converted to enum value",
               audioParamStr.c_str() );

  const Json::Value& mapJson = inJson["values"];
  for( auto mapIt = mapJson.begin(); mapIt != mapJson.end(); ++mapIt ) {
    SimpleMoodType simpleMood;
    if( ANKI_VERIFY( SimpleMoodTypeFromString( mapIt.key().asCString(), simpleMood ),
                     "MoodManager.LoadAudioSimpleMoodMap.InvalidSimpleMood",
                     "event map key '%s' is not a valid simple mood",
                     mapIt.key().asCString() ) ) {
      if( ANKI_VERIFY( mapIt->isNumeric(),
                       "MoodManager.LoadAudioSimpleMoodMap.InvalidSimpleValue",
                       "event map key '%s' does not map to numeric value",
                       mapIt.key().asCString() ) ) {
        
        _simpleMoodAudioEventMap.emplace( simpleMood, mapIt->asFloat() );
      }
    }
  }  
}

void MoodManager::LoadActionCompletedEventMap(const Json::Value& inJson)
{
  if( ! inJson.isNull() ) {

    for(auto actionIt = inJson.begin(); actionIt != inJson.end(); ++actionIt) {
      if( ! actionIt->isNull() ) {

        RobotActionType actionType = RobotActionTypeFromString(actionIt.key().asCString());

        for(auto resultIt = actionIt->begin(); resultIt != actionIt->end(); ++resultIt) {
          if( ! resultIt->isNull() ) {

            ActionResultCategory resultCategory = ActionResultCategoryFromString(resultIt.key().asCString());
            _actionCompletedEventMap.insert( { {actionType, resultCategory}, resultIt->asString() } );
          }
        }
      }
    }
  }

  PRINT_CH_INFO("Mood", "MoodManager.LoadedEventMap",
                "Loaded mood reactions for %zu (actionType, resultCategory) pairs",
                _actionCompletedEventMap.size());

  // PrintActionCompletedEventMap();
}

void MoodManager::PrintActionCompletedEventMap() const
{
  PRINT_CH_INFO("Mood", "MoodManager.PrintActionCompletedEventMap", "action result event map follows:");

  for( const auto& it : _actionCompletedEventMap ) {
    PRINT_CH_INFO("Mood", "MoodManager.PrintActionCompletedEventMap",
                  "%20s %15s %s",
                  RobotActionTypeToString(it.first.first),
                  ActionResultCategoryToString(it.first.second),
                  it.second.c_str());
  }
}

void MoodManager::Reset()
{
  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    GetEmotionByIndex(i).Reset();
  }
  _lastUpdateTime = 0.0f;
}


void MoodManager::UpdateDependent(const RobotCompMap& dependentComps)
{
  ANKI_CPU_PROFILE("MoodManager::Update");
  const float currentTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  const float kMinTimeStep = 0.0001f; // minimal sensible timestep, should be at least > epsilon
  float timeDelta = (_lastUpdateTime != 0.0f) ? (currentTime - _lastUpdateTime) : kMinTimeStep;
  if (timeDelta < kMinTimeStep)
  {
    PRINT_NAMED_WARNING("MoodManager.BadTimeStep", "TimeStep %f (%f-%f) is < %f - clamping!", timeDelta, currentTime, _lastUpdateTime, kMinTimeStep);
    timeDelta = kMinTimeStep;
  }

  timeDelta *= kTimeMultiplier;

  _lastUpdateTime = currentTime;

  SEND_MOOD_TO_VIZ_DEBUG_ONLY( VizInterface::RobotMood robotMood );
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( robotMood.emotion.reserve((size_t)EmotionType::Count) );
  
  float stimulatedValue = 0.0f;
  float stimulatedRate = 0.0f;
  float stimulatedAccel = 0.0f;
  bool foundStim = false;

  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    const EmotionType emotionType = (EmotionType)i;
    Emotion& emotion = GetEmotionByIndex(i);

    float rate = 0.0f;
    float accel = 0.0f;
    emotion.Update(GetStaticMoodData().GetDecayEvaluator(emotionType), timeDelta, rate, accel);
    
    SEND_MOOD_TO_VIZ_DEBUG_ONLY( robotMood.emotion.push_back(emotion.GetValue()) );
    
    if( emotionType == EmotionType::Stimulated ) {
      stimulatedValue = emotion.GetValue();
      stimulatedRate = rate;
      stimulatedAccel = accel;
      foundStim = true;
    }
  }

  const bool hasAudioComp = dependentComps.HasComponent<Audio::EngineRobotAudioClient>();
  if( hasAudioComp && ( (currentTime - _lastAudioSendTime_s) > kMoodManager_AudioSendPeriod_s ) ) {
    SendEmotionsToAudio(dependentComps.GetComponent<Audio::EngineRobotAudioClient>());
  }

  if( ANKI_DEV_CHEATS && dependentComps.HasComponent<ContextWrapper>() ) {
    if( (currentTime - _lastWebVizSendTime_s) > kMoodManager_WebVizPeriod_s ) {
      SendMoodToWebViz(dependentComps.GetComponent<ContextWrapper>().context);
    }
  }

  if( dependentComps.HasComponent<RobotStatsTracker>() ){

    // update stats tracker (integral of total stim)
    const float stimulated = GetEmotion(EmotionType::Stimulated).GetValue();
    const float delta = timeDelta * stimulated;
    if( delta > 0.0f ) {
      dependentComps.GetComponent<RobotStatsTracker>().IncreaseStimulationSeconds(delta);
    }

    if( _cumlPosStimDeltaToAdd > 0.0 ) {
      dependentComps.GetComponent<RobotStatsTracker>().IncreaseStimulationCumulativePositiveDelta(_cumlPosStimDeltaToAdd);
      _cumlPosStimDeltaToAdd = 0.0;
    }
  }
  
  SendEmotionsToGame();
  
  if( !_pendingAppEvents.empty() || (currentTime - _lastAppSentStimTime_s >= kMoodManager_AppPeriod_s) ) {
    // this won't send a new stim rate/accel the moment it changes, but since this is currently the only
    // user-facing viz of stim, that's ok. It _will_ send a new stim/rate accel when an event affects it,
    // and periodically
    DEV_ASSERT( foundStim, "MoodManager.UpdateDependent.NoStim" );
    SendStimToApp( stimulatedRate, stimulatedAccel );
  }
  _lastStimValue = stimulatedValue;

  #if SEND_MOOD_TO_VIZ_DEBUG
  robotMood.recentEvents = std::move(_eventNames);
  _eventNames.clear();

  // Can have null robot for unit tests
  if ((nullptr != _robot) &&
      _robot->HasComponent<ContextWrapper>() &&
      kSendMoodToViz)
  {
    _robot->GetContext()->GetVizManager()->SendRobotMood(std::move(robotMood));
  }
  #endif //SEND_MOOD_TO_VIZ_DEBUG
}

void MoodManager::SendMoodToWebViz(const CozmoContext* context, const std::string& emotionEvent)
{
  if( nullptr == context ) {
    return;
  }

  const float currentTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  Json::Value data;
  data["time"] = currentTime_s;

  auto& moodData = data["moods"];

  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    const EmotionType emotionType = (EmotionType)i;
    const float val = GetEmotionByIndex(i).GetValue();

    Json::Value entry;
    entry["emotion"] = EmotionTypeToString( emotionType );
    entry["value"] = val;
    moodData.append( entry );
  }

  if( ! emotionEvent.empty() ) {
    data["emotionEvent"] = emotionEvent;
  }

  data["simpleMood"] = SimpleMoodTypeToString(GetSimpleMood());

  const auto* web = context->GetWebService();
  if( nullptr != web ) {
    web->SendToWebViz( kWebVizModuleName, data );
  }

  _lastWebVizSendTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

void MoodManager::HandleActionEnded(const ExternalInterface::RobotCompletedAction& completion)
{
  auto ignoreIt = _actionsTagsToIgnore.find(completion.idTag);
  if( ignoreIt != _actionsTagsToIgnore.end() ) {
    _actionsTagsToIgnore.erase(ignoreIt);
    return;
  }


  ActionResultCategory resultCategory = IActionRunner::GetActionResultCategory(completion.result);

  auto it = _actionCompletedEventMap.find( {completion.actionType, resultCategory} );
  if( it != _actionCompletedEventMap.end() ) {
    PRINT_CH_DEBUG("Mood", "MoodManager.ActionCompleted.Reaction",
                   "Reacting to action of type '%s' completion with category '%s' by triggering event '%s'",
                   RobotActionTypeToString(completion.actionType),
                   ActionResultCategoryToString(resultCategory),
                   it->second.c_str());

    TriggerEmotionEvent(it->second, GetCurrentTimeInSeconds());
  }
}

template<>
void MoodManager::HandleMessage(const ExternalInterface::MoodMessage& msg)
{
  const Anki::Cozmo::ExternalInterface::MoodMessageUnion& moodMessage = msg.MoodMessageUnion;
  switch (moodMessage.GetTag())
  {
    case ExternalInterface::MoodMessageUnionTag::GetEmotions:
      SendEmotionsToGame();
      break;
    case ExternalInterface::MoodMessageUnionTag::SetEmotion:
    {
      const Anki::Cozmo::ExternalInterface::SetEmotion& msg = moodMessage.Get_SetEmotion();
      SetEmotion(msg.emotionType, msg.newVal);
      break;
    }
    case ExternalInterface::MoodMessageUnionTag::AddToEmotion:
    {
      const Anki::Cozmo::ExternalInterface::AddToEmotion& msg = moodMessage.Get_AddToEmotion();
      AddToEmotion(msg.emotionType, msg.deltaVal, msg.uniqueIdString.c_str(), GetCurrentTimeInSeconds());
      break;
    }
    case ExternalInterface::MoodMessageUnionTag::TriggerEmotionEvent:
    {
      const Anki::Cozmo::ExternalInterface::TriggerEmotionEvent& msg = moodMessage.Get_TriggerEmotionEvent();
      TriggerEmotionEvent(msg.emotionEventName, GetCurrentTimeInSeconds());
      break;
    }
    default:
      PRINT_NAMED_ERROR("MoodManager.HandleMoodEvent.UnhandledMessageUnionTag", "Unexpected tag %u", (uint32_t)moodMessage.GetTag());
      assert(0);
  }
}


void MoodManager::SendEmotionsToGame()
{
  ANKI_CPU_PROFILE("MoodManager::SendEmotionsToGame"); // ~1ms per tick (iPhone 5S in Release) - could send every N ticks?

  if (_robot)
  {
    std::vector<float> emotionValues;
    emotionValues.reserve((size_t)EmotionType::Count);

    for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
    {
      const Emotion& emotion = GetEmotionByIndex(i);
      emotionValues.push_back(emotion.GetValue());
    }

    ExternalInterface::MoodState message(std::move(emotionValues));
    _robot->Broadcast(ExternalInterface::MessageEngineToGame(std::move(message)));
  }
}

void MoodManager::SendEmotionsToAudio(Audio::EngineRobotAudioClient& audioClient)
{
  _lastAudioSendTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();

  // disabled for PR demo (let audio use it's own custom settings here)
  if( nullptr != _robot ) {
    const auto* featureGate = _robot->GetContext()->GetFeatureGate();
    const bool isPrDemo = featureGate->IsFeatureEnabled(Anki::Cozmo::FeatureType::PRDemo);
    if(isPrDemo) {
      return;
    }
  }

  if( !_audioParameterMap.empty() ) {
    for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
    {
      const EmotionType emotionType = (EmotionType)i;
      Emotion& emotion = GetEmotionByIndex(i);

      auto audioParamIt = _audioParameterMap.find(emotionType);
      if( audioParamIt != _audioParameterMap.end() ) {
        const float val = emotion.GetValue();
        audioClient.PostParameter( audioParamIt->second, val );
      }
    }
  }
  
  if( !_simpleMoodAudioEventMap.empty() ) {
    auto simpleMoodIt = _simpleMoodAudioEventMap.find( GetSimpleMood() );
    if( simpleMoodIt != _simpleMoodAudioEventMap.end() ) {
      const float val = simpleMoodIt->second;
      audioClient.PostParameter( _simpleMoodAudioParameter, val );
    }
  }  
}
  
void MoodManager::SendStimToApp(float velocity, float accel)
{
  const auto& emotion = GetEmotion( EmotionType::Stimulated );
  const float value = emotion.GetValue();
  if( (_robot != nullptr) && _robot->HasGatewayInterface() ) {
    auto* gi = _robot->GetGatewayInterface();
  
    external_interface::StimulationInfo* msg = new external_interface::StimulationInfo;
    // todo: extend proto plugin to take repeated types so this can be done in the ctor, to ensure we fill out all params
    *msg->mutable_emotion_events() = {_pendingAppEvents.begin(), _pendingAppEvents.end()};
    msg->set_value( value );
    msg->set_velocity( velocity );
    msg->set_accel( accel );
    msg->set_value_before_event( _pendingAppEvents.empty() ? value : _lastStimValue );
    msg->set_min_value( emotion.GetMin() );
    msg->set_max_value( emotion.GetMax() );
    
    gi->Broadcast( ExternalMessageRouter::Wrap( msg ) );
  }
  
  _lastAppSentStimTime_s = GetCurrentTimeInSeconds();
  _pendingAppEvents.clear();
}

// updates the most recent time this event was triggered, and returns how long it's been since the event was last seen
// returns FLT_MAX if this is the first time the event has been seen
float MoodManager::UpdateLatestEventTimeAndGetTimeElapsedInSeconds(const std::string& eventName, float currentTimeInSeconds)
{
  auto newEntry = _moodEventTimes.insert( MoodEventTimes::value_type(eventName, currentTimeInSeconds) );

  if (newEntry.second)
  {
    // first time event has occurred, map insert has successfully updated the time seen
    return FLT_MAX;
  }
  else
  {
    // event has happened before - calculate time since it last occurred and the matching penalty, then update the time

    float& timeEventLastOccurred = newEntry.first->second;
    const float timeSinceLastOccurrence = Util::numeric_cast<float>(currentTimeInSeconds - timeEventLastOccurred);

    timeEventLastOccurred = currentTimeInSeconds;

    return timeSinceLastOccurrence;
  }
}


float MoodManager::UpdateEventTimeAndCalculateRepetitionPenalty(const std::string& eventName, float currentTimeInSeconds)
{
  const float timeSinceLastOccurrence = UpdateLatestEventTimeAndGetTimeElapsedInSeconds(eventName, currentTimeInSeconds);

  const EmotionEvent* emotionEvent = GetStaticMoodData().GetEmotionEventMapper().FindEvent(eventName);

  if (emotionEvent)
  {
    // Use the emotionEvent with the matching name for calculating the repetition penalty
    const auto& defaultPenalty = GetStaticMoodData().GetDefaultRepetitionPenalty();
    const float repetitionPenalty = emotionEvent->CalculateRepetitionPenalty(timeSinceLastOccurrence, defaultPenalty);
    return repetitionPenalty;
  }
  else
  {
    PRINT_NAMED_WARNING("MoodManager.UpdateEventTimeAndCalculateRepetitionPenalty.NoEvent",
                        "Could not find event '%s', using default repetition penalty",
                        eventName.c_str());

    // No matching event name - use the default repetition penalty
    const float repetitionPenalty = GetStaticMoodData().GetDefaultRepetitionPenalty().EvaluateY(timeSinceLastOccurrence);
    return repetitionPenalty;
  }
}


bool MoodManager::IsValidEmotionEvent(const std::string& eventName) const
{
  const EmotionEvent* emotionEvent = GetStaticMoodData().GetEmotionEventMapper().FindEvent(eventName);
  return nullptr != emotionEvent;
}

void MoodManager::TriggerEmotionEvent(const std::string& eventName)
{
  TriggerEmotionEvent(eventName, GetCurrentTimeInSeconds());
}

void MoodManager::TriggerEmotionEvent(const std::string& eventName, float currentTimeInSeconds)
{
  const EmotionEvent* emotionEvent = GetStaticMoodData().GetEmotionEventMapper().FindEvent(eventName);
  if (emotionEvent)
  {
    PRINT_CH_INFO("Mood", "TriggerEmotionEvent", "%s", eventName.c_str());

    // update webviz before the event
    if( ANKI_DEV_CHEATS && kMoodManager_WebVizPeriod_s >= 0.0f && nullptr != _robot ) {
      SendMoodToWebViz(_robot->GetContext());
    }

    const float timeSinceLastOccurrence = UpdateLatestEventTimeAndGetTimeElapsedInSeconds(eventName, currentTimeInSeconds);
    const auto& defaultPenalty = GetStaticMoodData().GetDefaultRepetitionPenalty();
    const float repetitionPenalty = emotionEvent->CalculateRepetitionPenalty(timeSinceLastOccurrence, defaultPenalty);
    
    bool modified = false;

    const std::vector<EmotionAffector>& emotionAffectors = emotionEvent->GetAffectors();
    for (const EmotionAffector& emotionAffector : emotionAffectors)
    {
      const float penalizedDeltaValue = emotionAffector.GetValue() * repetitionPenalty;
      if( !IsEmotionFixed( emotionAffector.GetType() ) ) {
        modified = true;
        GetEmotion(emotionAffector.GetType()).Add(penalizedDeltaValue);
        
        if( emotionAffector.GetType() == EmotionType::Stimulated ) {
          // for stats tracking in the update loop
          _cumlPosStimDeltaToAdd += penalizedDeltaValue;
          // will get sent to the app the next Update() tick
          _pendingAppEvents.push_back( eventName );
        }
      } else {
        PRINT_CH_INFO("Mood", "MoodManager.TriggerFixedEmotion",
                      "Skipping TriggerEmotionEvent for emotion '%s' since it's fixed",
                      EmotionTypeToString(emotionAffector.GetType()));
      }      
    }

    if( modified ) {
      if( _robot ) {
        // update audio now instead of waiting for the next natural update
        SendEmotionsToAudio(_robot->GetComponent<Audio::EngineRobotAudioClient>());
      }

      SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(eventName.c_str()) );

      // Trying to answer the question of why emotions are changing
      std::ostringstream stream;
      std::for_each(_emotions, _emotions+((size_t)(EmotionType::Count)),
                    [&stream](const Emotion &iter){ stream<<iter.GetValue(); stream<<","; });
      Anki::Util::sInfo("robot.mood_values", {{DDATA,eventName.c_str()}}, stream.str().c_str());

      // and update webviz after, with the name of the event that happened
      if( ANKI_DEV_CHEATS && kMoodManager_WebVizPeriod_s >= 0.0f && nullptr != _robot ) {
        SendMoodToWebViz(_robot->GetContext(), eventName);
      }
    }
  }
  else
  {
    PRINT_NAMED_ERROR("MoodManager.TriggerEmotionEvent.EventNotFound", "Failed to find event '%s'", eventName.c_str());
  }
}


void MoodManager::AddToEmotion(EmotionType emotionType, float baseValue, const char* uniqueIdString, float currentTimeInSeconds)
{
  if( !IsEmotionFixed( emotionType ) ) {
    const float repetitionPenalty = UpdateEventTimeAndCalculateRepetitionPenalty(uniqueIdString, currentTimeInSeconds);
    const float penalizedDeltaValue = baseValue * repetitionPenalty;
    GetEmotion(emotionType).Add(penalizedDeltaValue);
    SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(uniqueIdString) );
  } else {
    PRINT_CH_INFO("Mood", "MoodManager.AddToFixedEmotion",
                  "Skipping AddToEmotion since emotion '%s' is fixed",
                  EmotionTypeToString(emotionType));
  }
}


void MoodManager::AddToEmotions(EmotionType emotionType1, float baseValue1,
                                EmotionType emotionType2, float baseValue2, const char* uniqueIdString, float currentTimeInSeconds)
{
  const float repetitionPenalty = UpdateEventTimeAndCalculateRepetitionPenalty(uniqueIdString, currentTimeInSeconds);
  bool modified = false;
  if( !IsEmotionFixed( emotionType1 ) ) {
    modified = true;
    const float penalizedDeltaValue1 = baseValue1 * repetitionPenalty;
    GetEmotion(emotionType1).Add(penalizedDeltaValue1);
  }
  
  if( !IsEmotionFixed( emotionType2 ) ) {
    modified = true;
    const float penalizedDeltaValue2 = baseValue2 * repetitionPenalty;
    GetEmotion(emotionType2).Add(penalizedDeltaValue2);
  }

  if( modified ) {
    SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(uniqueIdString) );
  } else {
    PRINT_CH_INFO("Mood", "MoodManager.AddToFixedEmotions2",
                  "AddToEmotions use with emotions '%s' and '%s' that are fixed= %d,%d",
                  EmotionTypeToString(emotionType1), EmotionTypeToString(emotionType2),
                  IsEmotionFixed(emotionType1), IsEmotionFixed(emotionType2));
  }
}


void MoodManager::AddToEmotions(EmotionType emotionType1, float baseValue1,
                                EmotionType emotionType2, float baseValue2,
                                EmotionType emotionType3, float baseValue3, const char* uniqueIdString, float currentTimeInSeconds)
{
  const float repetitionPenalty = UpdateEventTimeAndCalculateRepetitionPenalty(uniqueIdString, currentTimeInSeconds);
  bool modified = false;
  if( !IsEmotionFixed( emotionType1 ) ) {
    modified = true;
    const float penalizedDeltaValue1 = baseValue1 * repetitionPenalty;
    GetEmotion(emotionType1).Add(penalizedDeltaValue1);
  }
  
  if( !IsEmotionFixed( emotionType2 ) ) {
    modified = true;
    const float penalizedDeltaValue2 = baseValue2 * repetitionPenalty;
    GetEmotion(emotionType2).Add(penalizedDeltaValue2);
  }
  
  if( !IsEmotionFixed( emotionType3 ) ) {
    modified = true;
    const float penalizedDeltaValue3 = baseValue3 * repetitionPenalty;
    GetEmotion(emotionType3).Add(penalizedDeltaValue3);
  }

  if( modified ) {
    SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(uniqueIdString) );
  } else {
    PRINT_CH_INFO("Mood", "MoodManager.AddToFixedEmotions3",
                  "AddToEmotions use with emotions '%s','%s','%s' that are fixed = %d,%d,%d",
                  EmotionTypeToString(emotionType1), EmotionTypeToString(emotionType2), EmotionTypeToString(emotionType3),
                  IsEmotionFixed(emotionType1), IsEmotionFixed(emotionType2), IsEmotionFixed(emotionType3));
  }
}


void MoodManager::SetEmotion(EmotionType emotionType, float value)
{
  GetEmotion(emotionType).SetValue(value);
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent("SetEmotion") );
  if( ANKI_DEV_CHEATS && kMoodManager_WebVizPeriod_s >= 0.0f && nullptr != _robot ) {
    SendMoodToWebViz(_robot->GetContext(), "SetEmotion");
  }
}

void MoodManager::SetEnableMoodEventOnCompletion(u32 actionTag, bool enable)
{
  if( enable ) {
    _actionsTagsToIgnore.erase(actionTag);
  }
  else {
    _actionsTagsToIgnore.insert(actionTag);
  }
}

SimpleMoodType MoodManager::GetSimpleMood() const
{
  const float stimulated = GetEmotion(EmotionType::Stimulated).GetValue();
  const float confident  = GetEmotion(EmotionType::Confident ).GetValue();
  SimpleMoodType ret = GetSimpleMood( stimulated, confident );
  return ret;
}

SimpleMoodType MoodManager::GetSimpleMood(float stimulated, float confident)
{
  // low stim takes precedence because we don't want to annoy people
  if( stimulated <= 0.2f ) {
    return SimpleMoodType::LowStim;
  }

  // next priority is frustration
  if( confident < -0.29f ) { // this is the value used on Cozmo, may need tuning
    return SimpleMoodType::Frustrated;
  }

  if( stimulated <= 0.8 ) {
    return SimpleMoodType::MedStim;
  }
  else {
    return SimpleMoodType::HighStim;
  }

  // Note: never returns Default (default is used by the animation group selector if more specific anims
  // aren't provided)
}

bool MoodManager::DidSimpleMoodTransitionThisTick(SimpleMoodType from, SimpleMoodType to) const
{
  bool ret = false;
  SimpleMoodType currMood = GetSimpleMood();
  if( currMood == to ) {
    const float prevStim = GetEmotion( EmotionType::Stimulated ).GetHistoryValueTicksAgo( 1 );
    const float prevConf  = GetEmotion( EmotionType::Confident ).GetHistoryValueTicksAgo( 1 );
    SimpleMoodType oldMood = GetSimpleMood( prevStim, prevConf );
    ret = (oldMood != currMood) && (oldMood == from);
  }
  return ret;
}

bool MoodManager::DidSimpleMoodTransitionThisTickFrom(SimpleMoodType from) const
{
  const float prevStim = GetEmotion( EmotionType::Stimulated ).GetHistoryValueTicksAgo( 1 );
  const float prevConf  = GetEmotion( EmotionType::Confident ).GetHistoryValueTicksAgo( 1 );
  SimpleMoodType oldMood = GetSimpleMood( prevStim, prevConf );
  SimpleMoodType currMood = GetSimpleMood();
  const bool ret = (oldMood != currMood) && (oldMood == from);
  return ret;
}

bool MoodManager::DidSimpleMoodTransitionThisTick() const
{
  const float prevStim = GetEmotion( EmotionType::Stimulated ).GetHistoryValueTicksAgo( 1 );
  const float prevConf  = GetEmotion( EmotionType::Confident ).GetHistoryValueTicksAgo( 1 );
  SimpleMoodType oldMood = GetSimpleMood( prevStim, prevConf );
  SimpleMoodType currMood = GetSimpleMood();
  const bool ret = (oldMood != currMood);
  return ret;
}

void MoodManager::SubscribeToWebViz()
{
  if( _robot == nullptr ) {
    return;
  }
  const auto* context = _robot->GetContext();
  if( context == nullptr ) {
    return;
  }
  auto* webService = context->GetWebService();
  if( webService == nullptr ) {
    return;
  }

  auto onSubscribedBehaviors = [this](const std::function<void(const Json::Value&)>& sendToClient) {
    // a client subscribed. send them min/max values for each emotion, and a list of SimpleMoods
    // and arbitrary emotion values for each SimpleMood

    Json:: Value subscriptionData;
    auto& data = subscriptionData["info"];
    auto& emotions = data["emotions"];
    for( uint8_t i=0; i<static_cast<uint8_t>(EmotionType::Count); ++i ) {
      Json::Value emotionEntry;
      auto emotion = static_cast<EmotionType>(i);
      emotionEntry["emotionType"] = EmotionTypeToString(emotion);

      const auto& map = GetStaticMoodData().GetEmotionValueRangeMap();
      auto it = map.find(emotion);
      if( it != map.end() ) {
        emotionEntry["min"] = it->second.first;
        emotionEntry["max"] = it->second.second;
      } else {
        emotionEntry["min"] = kEmotionValueMin;
        emotionEntry["max"] = kEmotionValueMax;
      }
      emotions.append( emotionEntry );
    }
    auto& simpleMoods = data["simpleMoods"];
    // some example values that, if received, would transition to the given simple mood.
    // only some emotions are provided, so that only those will be sent back, leaving others unchanged
    simpleMoods["LowStim"]["Stimulated"] = 0.1;
    simpleMoods["MedStim"]["Confident"] = -0.1;
    simpleMoods["MedStim"]["Stimulated"] = 0.5;
    simpleMoods["HighStim"]["Confident"] = -0.1;
    simpleMoods["HighStim"]["Stimulated"] = 0.9;
    simpleMoods["Frustrated"]["Confident"] = -0.4;
    simpleMoods["Frustrated"]["Stimulated"] = 0.3;

    // do some verification that the above numbers are still valid
    {
      static_assert( SimpleMoodTypeNumEntries == 6, "Please update the values above with the new simple moods.");
      const auto moodList = {SimpleMoodType::LowStim, SimpleMoodType::MedStim, SimpleMoodType::HighStim, SimpleMoodType::Frustrated};
      for( const auto& simpleMood : moodList )  {
        const float stim = simpleMoods[SimpleMoodTypeToString(simpleMood)]["Stimulated"].isDouble()
                             ? simpleMoods[SimpleMoodTypeToString(simpleMood)]["Stimulated"].asDouble()
                             : 0.0f;
        const float conf = simpleMoods[SimpleMoodTypeToString(simpleMood)]["Confident"].isDouble()
                             ? simpleMoods[SimpleMoodTypeToString(simpleMood)]["Confident"].asDouble()
                             : 0.0f;
        ANKI_VERIFY( (GetSimpleMood( stim, conf ) == simpleMood)
                     && (stim >= GetStaticMoodData().GetEmotionValueRangeMap().at(EmotionType::Stimulated).first)
                     && (stim <= GetStaticMoodData().GetEmotionValueRangeMap().at(EmotionType::Stimulated).second)
                     && (conf >= -1.0f) && (conf <= 1.0f),
                     "MoodManager.SubscribeToWebViz.ValsNeedsUpdate",
                     "Update this dev method with new emotion/simplemood values" );
      }
    }
    sendToClient( subscriptionData );
  };

  auto onDataBehaviors = [this](const Json::Value& data, const std::function<void(const Json::Value&)>& sendToClient) {
    // if the client sent any emotion types, set them
    for( uint8_t i=0; i<EmotionTypeNumEntries; ++i ) {
      auto emotion = static_cast<EmotionType>(i);
      if( data[EmotionTypeToString(emotion)].isDouble() ) {
        SetEmotion( emotion, data[EmotionTypeToString(emotion)].asDouble() );
      }
    }
  };

  _signalHandles.emplace_back( webService->OnWebVizSubscribed( kWebVizModuleName ).ScopedSubscribe( onSubscribedBehaviors ) );
  _signalHandles.emplace_back( webService->OnWebVizData( kWebVizModuleName ).ScopedSubscribe( onDataBehaviors ) );
}

#if SEND_MOOD_TO_VIZ_DEBUG
void MoodManager::AddEvent(const char* eventName)
{
  if (_eventNames.empty() || (_eventNames.back() != eventName))
  {
    _eventNames.push_back(eventName);
  }
}
#endif // SEND_MOOD_TO_VIZ_DEBUG


} // namespace Cozmo
} // namespace Anki
