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
#include "engine/cozmoContext.h"
#include "engine/events/ankiEvent.h"
#include "engine/externalInterface/externalInterface.h"
#include "engine/moodSystem/emotionEvent.h"
#include "engine/moodSystem/staticMoodData.h"
#include "engine/robot.h"
#include "engine/viz/vizManager.h"
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

namespace {
static const char* kActionResultEmotionEventKey = "actionResultEmotionEvents";
static const char* kAudioParametersMapKey = "audioParameterMap";
  
CONSOLE_VAR(bool, kSendMoodToViz, "VizDebug", true);

CONSOLE_VAR(float, kAudioSendPeriod_s, "MoodManager", 0.5f);
CONSOLE_VAR(float, kWebVizPeriod_s, "MoodManager", 1.0f);
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
{
}

MoodManager::~MoodManager()
{
  // if the robot is destructing, it might not have an action list, so check that here
  if( _actionCallbackID != 0 && _robot != nullptr && _robot->HasComponent(RobotComponentID::ActionList) ) {
    _robot->GetActionList().UnregisterCallback(_actionCallbackID);
    _actionCallbackID = 0;
  }
}

void MoodManager::InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents)
{
  _robot = robot;
  auto& context = dependentComponents.GetValue<ContextWrapper>().context;

  if (nullptr != context->GetDataPlatform())
  {
    ReadMoodConfig(context->GetDataLoader()->GetRobotMoodConfig());
    LoadEmotionEvents(context->GetDataLoader()->GetEmotionEventJsons());
  }
}


void MoodManager::ReadMoodConfig(const Json::Value& inJson)
{
  GetStaticMoodData().Init(inJson);

  LoadAudioParameterMap(inJson[kAudioParametersMapKey]);
  LoadActionCompletedEventMap(inJson[kActionResultEmotionEventKey]);  

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
  
  _lastUpdateTime = currentTime;

  SEND_MOOD_TO_VIZ_DEBUG_ONLY( VizInterface::RobotMood robotMood );
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( robotMood.emotion.reserve((size_t)EmotionType::Count) );

  for (size_t i = 0; i < (size_t)EmotionType::Count; ++i)
  {
    const EmotionType emotionType = (EmotionType)i;
    Emotion& emotion = GetEmotionByIndex(i);
    
    emotion.Update(GetStaticMoodData().GetDecayGraph(emotionType), currentTime, timeDelta);

    SEND_MOOD_TO_VIZ_DEBUG_ONLY( robotMood.emotion.push_back(emotion.GetValue()) );
  }

  const bool hasAudioComp = dependentComps.HasComponent(RobotComponentID::EngineAudioClient);
  if( hasAudioComp && ( (currentTime - _lastAudioSendTime_s) > kAudioSendPeriod_s ) ) {
    SendEmotionsToAudio(dependentComps.GetValue<Audio::EngineRobotAudioClient>());
  }

  if( ANKI_DEV_CHEATS && dependentComps.HasComponent(RobotComponentID::CozmoContextWrapper) ) {
    if( (currentTime - _lastWebVizSendTime_s) > kWebVizPeriod_s ) {
      SendMoodToWebViz(dependentComps.GetValue<ContextWrapper>().context);
    }
  }
  
  SendEmotionsToGame();

  #if SEND_MOOD_TO_VIZ_DEBUG
  robotMood.recentEvents = std::move(_eventNames);
  _eventNames.clear();
  
  // Can have null robot for unit tests
  if ((nullptr != _robot) &&
      _robot->HasComponent(RobotComponentID::CozmoContextWrapper) &&
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

  _lastAudioSendTime_s = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
}

  
// updates the most recent time this event was triggered, and returns how long it's been since the event was last seen
// returns FLT_MAX if this is the first time the event has been seen
float MoodManager::UpdateLatestEventTimeAndGetTimeElapsedInSeconds(const std::string& eventName, float currentTimeInSeconds)
{
  auto newEntry = _moodEventTimes.insert( MoodEventTimes::value_type(eventName, currentTimeInSeconds) );
  
  if (newEntry.second)
  {
    // first time event has occured, map insert has successfully updated the time seen
    return FLT_MAX;
  }
  else
  {
    // event has happened before - calculate time since it last occured and the matching penalty, then update the time
    
    float& timeEventLastOccured = newEntry.first->second;
    const float timeSinceLastOccurence = Util::numeric_cast<float>(currentTimeInSeconds - timeEventLastOccured);
    
    timeEventLastOccured = currentTimeInSeconds;
    
    return timeSinceLastOccurence;
  }
}


float MoodManager::UpdateEventTimeAndCalculateRepetitionPenalty(const std::string& eventName, float currentTimeInSeconds)
{
  const float timeSinceLastOccurence = UpdateLatestEventTimeAndGetTimeElapsedInSeconds(eventName, currentTimeInSeconds);
  
  const EmotionEvent* emotionEvent = GetStaticMoodData().GetEmotionEventMapper().FindEvent(eventName);
  
  if (emotionEvent)
  {
    // Use the emotionEvent with the matching name for calculating the repetition penalty
    const float repetitionPenalty = emotionEvent->CalculateRepetitionPenalty(timeSinceLastOccurence);
    return repetitionPenalty;
  }
  else
  {
    // No matching event name - use the default repetition penalty
    const float repetitionPenalty = GetStaticMoodData().GetDefaultRepetitionPenalty().EvaluateY(timeSinceLastOccurence);
    return repetitionPenalty;
  }
}


void MoodManager::TriggerEmotionEvent(const std::string& eventName, float currentTimeInSeconds)
{
  const EmotionEvent* emotionEvent = GetStaticMoodData().GetEmotionEventMapper().FindEvent(eventName);
  if (emotionEvent)
  {
    PRINT_CH_INFO("Mood", "TriggerEmotionEvent", "%s", eventName.c_str());
    
    // update webviz before the event
    if( ANKI_DEV_CHEATS && kWebVizPeriod_s >= 0.0f && nullptr != _robot ) {
      SendMoodToWebViz(_robot->GetContext());
    }

    const float timeSinceLastOccurence = UpdateLatestEventTimeAndGetTimeElapsedInSeconds(eventName, currentTimeInSeconds);
    const float repetitionPenalty = emotionEvent->CalculateRepetitionPenalty(timeSinceLastOccurence);

    const std::vector<EmotionAffector>& emotionAffectors = emotionEvent->GetAffectors();
    for (const EmotionAffector& emotionAffector : emotionAffectors)
    {
      const float penalizedDeltaValue = emotionAffector.GetValue() * repetitionPenalty;
      GetEmotion(emotionAffector.GetType()).Add(penalizedDeltaValue);
    }

    if( _robot ) {
      // update audio now instead of waiting for the next natural update
      SendEmotionsToAudio(_robot->GetComponent<Audio::EngineRobotAudioClient>());
    }
    
    SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(eventName.c_str()) );
    
    // Trying to answer the question of why emotions are changing
    std::ostringstream stream;
    std::for_each(_emotions, _emotions+((size_t)(EmotionType::Count)),
                  [&stream](const Emotion &iter){ stream<<iter.GetValue(); stream<<","; });
    Anki::Util::sEvent("robot.mood_values", {{DDATA,eventName.c_str()}}, stream.str().c_str());

    // and update webviz after, with the name of the event that happened
    if( ANKI_DEV_CHEATS && kWebVizPeriod_s >= 0.0f && nullptr != _robot ) {
      SendMoodToWebViz(_robot->GetContext(), eventName);
    }
  }
  else
  {
    PRINT_NAMED_WARNING("MoodManager.TriggerEmotionEvent.EventNotFound", "Failed to find event '%s'", eventName.c_str());
  }
}


void MoodManager::AddToEmotion(EmotionType emotionType, float baseValue, const char* uniqueIdString, float currentTimeInSeconds)
{
  const float repetitionPenalty = UpdateEventTimeAndCalculateRepetitionPenalty(uniqueIdString, currentTimeInSeconds);
  const float penalizedDeltaValue = baseValue * repetitionPenalty;
  GetEmotion(emotionType).Add(penalizedDeltaValue);
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(uniqueIdString) );
}


void MoodManager::AddToEmotions(EmotionType emotionType1, float baseValue1,
                                EmotionType emotionType2, float baseValue2, const char* uniqueIdString, float currentTimeInSeconds)
{
  const float repetitionPenalty = UpdateEventTimeAndCalculateRepetitionPenalty(uniqueIdString, currentTimeInSeconds);
  const float penalizedDeltaValue1 = baseValue1 * repetitionPenalty;
  const float penalizedDeltaValue2 = baseValue2 * repetitionPenalty;
  
  GetEmotion(emotionType1).Add(penalizedDeltaValue1);
  GetEmotion(emotionType2).Add(penalizedDeltaValue2);
  
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(uniqueIdString) );
}

  
void MoodManager::AddToEmotions(EmotionType emotionType1, float baseValue1,
                                EmotionType emotionType2, float baseValue2,
                                EmotionType emotionType3, float baseValue3, const char* uniqueIdString, float currentTimeInSeconds)
{
  const float repetitionPenalty = UpdateEventTimeAndCalculateRepetitionPenalty(uniqueIdString, currentTimeInSeconds);
  const float penalizedDeltaValue1 = baseValue1 * repetitionPenalty;
  const float penalizedDeltaValue2 = baseValue2 * repetitionPenalty;
  const float penalizedDeltaValue3 = baseValue3 * repetitionPenalty;
  
  GetEmotion(emotionType1).Add(penalizedDeltaValue1);
  GetEmotion(emotionType2).Add(penalizedDeltaValue2);
  GetEmotion(emotionType3).Add(penalizedDeltaValue3);
    
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent(uniqueIdString) );
}


void MoodManager::SetEmotion(EmotionType emotionType, float value)
{
  GetEmotion(emotionType).SetValue(value);
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( AddEvent("SetEmotion") );
  if( ANKI_DEV_CHEATS && kWebVizPeriod_s >= 0.0f && nullptr != _robot ) {
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
  float happiness = GetEmotion(EmotionType::Happy).GetValue();
  float confidence = GetEmotion(EmotionType::Confident).GetValue();
  // TODO:(bn) / mooly check AGs for driving groups, hopefully this will work for frustrated
  if(happiness < -0.33f || confidence < -0.29f) {
    return SimpleMoodType::Sad;
  }
  if(happiness > 0.33f) {
    return SimpleMoodType::Happy;
  }
  return SimpleMoodType::Default;
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

