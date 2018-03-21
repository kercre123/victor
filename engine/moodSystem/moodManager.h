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


#ifndef __Cozmo_Basestation_MoodSystem_MoodManager_H__
#define __Cozmo_Basestation_MoodSystem_MoodManager_H__

#include "coretech/common/shared/types.h"
#include "engine/moodSystem/emotion.h"
#include "engine/moodSystem/moodDebug.h"

#include "clad/types/actionResults.h"
#include "clad/types/actionTypes.h"
#include "clad/types/emotionTypes.h"
#include "clad/types/simpleMoodTypes.h"

#include "util/entityComponent/iDependencyManagedComponent.h"
#include "engine/moodSystem/emotion.h"
#include "engine/moodSystem/moodDebug.h"
#include "engine/robotComponents_fwd.h"
#include "engine/robotDataLoader.h"

#include "util/graphEvaluator/graphEvaluator2d.h"
#include "util/helpers/noncopyable.h"
#include "util/signals/simpleSignal_fwd.h"

#include <assert.h>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Anki {

namespace AudioMetaData {
namespace GameParameter {
// forward declaration (see audioParameterTypes.clad)
enum class ParameterType : u32;
}
}

namespace Cozmo {

constexpr float kEmotionChangeVerySmall = 0.06f;
constexpr float kEmotionChangeSmall     = 0.12f;
constexpr float kEmotionChangeMedium    = 0.25f;
constexpr float kEmotionChangeLarge     = 0.50f;
constexpr float kEmotionChangeVeryLarge = 1.00f;

  
template <typename Type>
class AnkiEvent;


namespace ExternalInterface {
class MessageGameToEngine;
struct RobotCompletedAction;
}

namespace Audio{
class EngineRobotAudioClient;
}
  
class CozmoContext;
class Robot;
class StaticMoodData;
  
class MoodManager : public IDependencyManagedComponent<RobotComponentID>, private Util::noncopyable
{
public:
  using MoodEventTimes = std::map<std::string, float>;
  
  explicit MoodManager();
  ~MoodManager();

  //////
  // IDependencyManagedComponent functions
  //////
  virtual void InitDependent(Cozmo::Robot* robot, const RobotCompMap& dependentComponents) override;
  virtual void AdditionalInitAccessibleComponents(RobotCompIDSet& components) const override {
    components.insert(RobotComponentID::CozmoContext);
  };
  virtual void GetUpdateDependencies(RobotCompIDSet& dependencies) const override {
    dependencies.insert(RobotComponentID::CozmoContext);
    dependencies.insert(RobotComponentID::EngineAudioClient);
  }
  virtual void UpdateDependent(const RobotCompMap& dependentComps) override;
  //////
  // end IDependencyManagedComponent functions
  //////
  
  // =========== Mood =============   
  
  void Reset();
    
  // ==================== Modify Emotions ====================
  
  void TriggerEmotionEvent(const std::string& eventName, float currentTimeInSeconds);
  
  void AddToEmotion(EmotionType emotionType, float baseValue, const char* uniqueIdString, float currentTimeInSeconds);
  
  void AddToEmotions(EmotionType emotionType1, float baseValue1,
                     EmotionType emotionType2, float baseValue2,
                     const char* uniqueIdString, float currentTimeInSeconds);
  
  void AddToEmotions(EmotionType emotionType1, float baseValue1,
                     EmotionType emotionType2, float baseValue2,
                     EmotionType emotionType3, float baseValue3,
                     const char* uniqueIdString, float currentTimeInSeconds);
  
  void SetEmotion(EmotionType emotionType, float value); // directly set the value e.g. for debugging

  // This manager internally listens for ActionCompleted events from the robot, and can use those to trigger
  // emotion events. By default, it listens for any actions that complete, but this function can be used to
  // enable or disable listening for specific actions
  void SetEnableMoodEventOnCompletion(u32 actionTag, bool enable);

  
  // ==================== GetEmotion... ====================

  float GetEmotionValue(EmotionType emotionType) const { return GetEmotion(emotionType).GetValue(); }

  float GetEmotionDeltaRecentTicks(EmotionType emotionType, uint32_t numTicksBackwards) const
  {
    return GetEmotion(emotionType).GetDeltaRecentTicks(numTicksBackwards);
  }
  
  float GetEmotionDeltaRecentSeconds(EmotionType emotionType, float secondsBackwards)   const
  {
    return GetEmotion(emotionType).GetDeltaRecentSeconds(secondsBackwards);
  }
  
  SimpleMoodType GetSimpleMood() const;
  
  float GetLastUpdateTime() const { return _lastUpdateTime; }
  
  // ==================== Event/Message Handling ====================
  // Handle various message types
  template<typename T>
  void HandleMessage(const T& msg);

  void HandleActionEnded(const ExternalInterface::RobotCompletedAction& completion);
  
  void SendEmotionsToGame();

  // ============================== Public Static Member Funcs ==============================
  
  static StaticMoodData& GetStaticMoodData();
  
  // Helper for anything calling functions that require currentTimeInSeconds, where they don't already have access to it
  static float GetCurrentTimeInSeconds();
  
private:
  void ReadMoodConfig(const Json::Value& inJson);
  // Load in all data-driven emotion events // TODO: move to mood manager?
  void LoadEmotionEvents(const RobotDataLoader::FileJsonMap& emotionEventData);   
  bool LoadEmotionEvents(const Json::Value& inJson);

  // ============================== Private Member Funcs ==============================
  
  float UpdateLatestEventTimeAndGetTimeElapsedInSeconds(const std::string& eventName, float currentTimeInSeconds);
  
  float UpdateEventTimeAndCalculateRepetitionPenalty(const std::string& eventName, float currentTimeInSeconds);
  
  void  FadeEmotionsToDefault(float delta);

  const Emotion&  GetEmotion(EmotionType emotionType) const { return GetEmotionByIndex((size_t)emotionType); }
  Emotion&        GetEmotion(EmotionType emotionType)       { return GetEmotionByIndex((size_t)emotionType); }
  
  const Emotion&  GetEmotionByIndex(size_t index) const
  {
    assert((index >= 0) && (index < (size_t)EmotionType::Count));
    return _emotions[index];
  }
  
  Emotion&  GetEmotionByIndex(size_t index)
  {
    assert((index >= 0) && (index < (size_t)EmotionType::Count));
    return _emotions[index];
  }

  void LoadAudioParameterMap(const Json::Value& inJson);
  void LoadActionCompletedEventMap(const Json::Value& inJson);
  void PrintActionCompletedEventMap() const;

  void SendEmotionsToAudio(Audio::EngineRobotAudioClient& audioClient);
  
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( void AddEvent(const char* eventName) );

  void SendMoodToWebViz(const CozmoContext* context, const std::string& emotionEvent = "");
    
  // ============================== Private Member Vars ==============================
  
  Emotion         _emotions[(size_t)EmotionType::Count];
  MoodEventTimes  _moodEventTimes;
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( std::vector<std::string> _eventNames; )
  Robot*          _robot = nullptr;
  float           _lastUpdateTime;

  // maps from (action type, action result category) -> emotion event
  using ActionCompletedEventMap = std::map< std::pair< RobotActionType, ActionResultCategory >, std::string >;
  ActionCompletedEventMap _actionCompletedEventMap;

  using AudioParameterType = AudioMetaData::GameParameter::ParameterType;  
  // map from emotion to audio parameter type to inform audio system of mood parameters
  std::map< EmotionType, AudioParameterType > _audioParameterMap;

  std::set< u32 > _actionsTagsToIgnore;
  
  std::vector<Signal::SmartHandle> _signalHandles;

  int _actionCallbackID = 0;

  float _lastAudioSendTime_s = 0.0f;
  float _lastWebVizSendTime_s = 0.0f;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_MoodManager_H__

