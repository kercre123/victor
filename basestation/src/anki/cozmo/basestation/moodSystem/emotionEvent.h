/**
 * File: emotionEvent
 *
 * Author: Mark Wesley
 * Created: 11/09/15
 *
 * Description: Moode/Emotion affecting event
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_EmotionEvent_H__
#define __Cozmo_Basestation_MoodSystem_EmotionEvent_H__


#include "clad/types/moodEventTypes.h"
#include "anki/cozmo/basestation/moodSystem/emotionAffector.h"
#include <vector>


namespace Json {
  class Value;
}

namespace Anki {
namespace Cozmo {

  
class EmotionEvent
{
public:
  
  explicit EmotionEvent(MoodEventType eventType = MoodEventType::Count);
  explicit EmotionEvent(const Json::Value& inJson);
  
  void AddEmotionAffector(const EmotionAffector& inAffector);
  
  MoodEventType GetEventType() const { return _eventType; }
  
  size_t GetNumAffectors() const { return _affectors.size(); }
  const EmotionAffector& GetAffector(size_t index) const { return _affectors[index]; }

  // ===== Json =====
  
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;
  
private:
  
  std::vector<EmotionAffector>  _affectors;
  MoodEventType                 _eventType;
};


} // namespace Cozmo
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_EmotionEvent_H__

