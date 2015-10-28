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
#include "util/graphEvaluator/graphEvaluator2d.h"
#include <assert.h>


namespace Anki {
namespace Cozmo {

  
constexpr float kEmotionChangeSmall  = 0.10f;
constexpr float kEmotionChangeMedium = 0.25f;
constexpr float kEmotionChangeLarge  = 0.50f;


class MoodManager
{
public:
  
  MoodManager();
  
  void Reset();
  
  void Update(double currentTime);
  
  void AddToEmotion(EmotionType emotionType, float baseValue, const char* uniqueIdString); // , bool dropOff?
  
  void SetEmotion(EmotionType emotionType, float value); // directly set the value e.g. for debugging

  float GetEmotionValue(EmotionType emotionType) const { return GetEmotion(emotionType).GetValue(); }

  float GetEmotionDeltaRecentTicks(EmotionType emotionType, uint32_t numTicksBackwards) const { return GetEmotion(emotionType).GetDeltaRecentTicks(numTicksBackwards); }
  float GetEmotionDeltaRecentSeconds(EmotionType emotionType, float secondsBackwards)   const { return GetEmotion(emotionType).GetDeltaRecentSeconds(secondsBackwards); }

  // ============================== Public Static Member Funcs ==============================
  
  static void  InitDecayGraphs();
  static void  SetDecayGraph(EmotionType emotionType, const Anki::Util::GraphEvaluator2d& newGraph);
  static const Anki::Util::GraphEvaluator2d& GetDecayGraph(EmotionType emotionType);

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
  
  // ============================== Private Static Member Vars ==============================

  static Anki::Util::GraphEvaluator2d    sEmotionDecayGraphs[(size_t)EmotionType::Count];
  
  // ============================== Private Member Vars ==============================
  
  Emotion     _emotions[(size_t)EmotionType::Count];
  double      _lastUpdateTime;
};
  

} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_MoodManager_H__

