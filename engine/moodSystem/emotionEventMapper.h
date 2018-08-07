/**
 * File: emotionEventMapper
 *
 * Author: Mark Wesley
 * Created: 11/16/15
 *
 * Description: Storage for EmotionEvents with lookups etc.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/


#ifndef __Cozmo_Basestation_MoodSystem_EmotionEventMapper_H__
#define __Cozmo_Basestation_MoodSystem_EmotionEventMapper_H__


#include <map>
#include <string>


namespace Json {
  class Value;
}


namespace Anki {
namespace Vector {

  
class EmotionEvent;
class MoodManager;
class Robot;

  
class EmotionEventMapper
{
public:
  
  EmotionEventMapper();
  ~EmotionEventMapper();
  
  // Explicitely delete copy and assignment operators (class doesn't support shallow copy)
  EmotionEventMapper(const EmotionEventMapper&) = delete;
  EmotionEventMapper& operator=(const EmotionEventMapper&) = delete;
  
  void Clear();
  
  void AddEvent(EmotionEvent* emotionEvent); // transfers ownership of event to EmotionEventMapper

  const EmotionEvent* FindEvent(const std::string& name) const;
  
  void ClearEvents();
  
  // ===== Json =====
  
  bool LoadEmotionEvent(const Json::Value& inJson);
  bool LoadEmotionEvents(const Json::Value& inJson);
  bool ReadFromJson(const Json::Value& inJson);
  bool WriteToJson(Json::Value& outJson) const;
  
private:
  
  using EventMap = std::map<std::string, EmotionEvent*>;
  
  EventMap  _events;
};
  

} // namespace Vector
} // namespace Anki


#endif // __Cozmo_Basestation_MoodSystem_EmotionEventMapper_H__

