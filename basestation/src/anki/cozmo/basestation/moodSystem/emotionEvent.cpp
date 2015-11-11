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


#include "anki/cozmo/basestation/moodSystem/emotionEvent.h"
#include "util/global/globalDefinitions.h"
#include "util/logging/logging.h"
#include "json/json.h"


namespace Anki {
namespace Cozmo {


EmotionEvent::EmotionEvent(MoodEventType eventType)
  : _eventType(eventType)
{
}
  
  
EmotionEvent::EmotionEvent(const Json::Value& inJson)
  : _eventType(kInvalidMoodEventType)
{
  ReadFromJson(inJson);
}
  
  
void EmotionEvent::AddEmotionAffector(const EmotionAffector& inAffector)
{
  #if ANKI_DEVELOPER_CODE
  for (const EmotionAffector& affector : _affectors)
  {
    if (affector.GetType() == inAffector.GetType())
    {
      PRINT_NAMED_WARNING("EmotionEvent.AddEmotionAffector.DuplicateType",
                          "Type '%s' already present (%f vs %f) - adding anyway!",
                          EnumToString(inAffector.GetType()), inAffector.GetValue(), affector.GetValue());
    }
  }
  #endif // ANKI_DEVELOPER_CODE

  _affectors.push_back(inAffector);
}


bool EmotionEvent::ReadFromJson(const Json::Value& inJson)
{
  // [MARKW:TODO] Implementation required
  
  return true;
}


bool EmotionEvent::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  // [MARKW:TODO] Implementation required
  
  return true;
}


} // namespace Cozmo
} // namespace Anki

