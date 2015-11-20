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


#include "anki/cozmo/basestation/moodSystem/emotion.h"
#include "clad/types/emotionTypes.h"
#include "util/global/globalDefinitions.h"
#include "util/graphEvaluator/graphEvaluator2d.h"
#include <assert.h>
#include <map>
#include <string>
#include <vector>


#if ANKI_DEV_CHEATS
  #define SEND_MOOD_TO_VIZ_DEBUG  1
#else
  #define SEND_MOOD_TO_VIZ_DEBUG  0
#endif // ANKI_DEV_CHEATS

#if SEND_MOOD_TO_VIZ_DEBUG
  #define SEND_MOOD_TO_VIZ_DEBUG_ONLY(exp)  exp
#else
  #define SEND_MOOD_TO_VIZ_DEBUG_ONLY(exp)
#endif // SEND_MOOD_TO_VIZ_DEBUG


namespace Anki {
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
}
  
  
class Robot;
class StaticMoodData;

  
class MoodManager
{
public:
  
  explicit MoodManager(Robot* inRobot = nullptr);
  
  void Init(const Json::Value& inJson);
  
  void Reset();
  
  void Update(double currentTime);
  
  // ==================== Modify Emotions ====================
  
  void TriggerEmotionEvent(const std::string& eventName);
  
  void AddToEmotion(EmotionType emotionType, float baseValue, const char* uniqueIdString); // , bool dropOff?
  
  void AddToEmotions(EmotionType emotionType1, float baseValue1,
                     EmotionType emotionType2, float baseValue2, const char* uniqueIdString);
  
  void AddToEmotions(EmotionType emotionType1, float baseValue1,
                     EmotionType emotionType2, float baseValue2,
                     EmotionType emotionType3, float baseValue3,
                     const char* uniqueIdString);
  
  void SetEmotion(EmotionType emotionType, float value); // directly set the value e.g. for debugging
  
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
  
  // ==================== Event/Message Handling ====================
  
  void HandleEvent(const AnkiEvent<ExternalInterface::MessageGameToEngine>& event);
  
  void SendEmotionsToGame();

  // ============================== Public Static Member Funcs ==============================
  
  static StaticMoodData& GetStaticMoodData();
  
private:
  
  // ============================== Private Member Funcs ==============================
  
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
  
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( void AddEvent(const char* eventName) );
    
  // ============================== Private Member Vars ==============================
  
  Emotion     _emotions[(size_t)EmotionType::Count];
  SEND_MOOD_TO_VIZ_DEBUG_ONLY( std::vector<std::string> _eventNames; )
  Robot*      _robot;
  double      _lastUpdateTime;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_MoodManager_H__

