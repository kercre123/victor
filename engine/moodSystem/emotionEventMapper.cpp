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


#include "engine/moodSystem/emotionEventMapper.h"
#include "engine/moodSystem/emotionEvent.h"
#include "json/json.h"
#include "util/logging/logging.h"


namespace Anki {
namespace Vector {


EmotionEventMapper::EmotionEventMapper()
{
}


EmotionEventMapper::~EmotionEventMapper()
{
  Clear();
}

  
void EmotionEventMapper::Clear()
{
  ClearEvents();
}


void EmotionEventMapper::ClearEvents()
{
  for (auto& kv : _events)
  {
    EmotionEvent* event = kv.second;
    delete event;
  }
  _events.clear();
}

  
const EmotionEvent* EmotionEventMapper::FindEvent(const std::string& name) const
{
  EventMap::const_iterator it = _events.find(name);
  if (it != _events.end())
  {
    return it->second;
  }
  
  return nullptr;
}


void EmotionEventMapper::AddEvent(EmotionEvent* emotionEvent)
{
  assert(emotionEvent);
  
  const std::string& eventName = emotionEvent->GetName();
  EventMap::iterator it = _events.find(eventName);
  
  if (it != _events.end())
  {
    EmotionEvent* oldEvent = it->second;
    PRINT_NAMED_WARNING("EmotionEventMapper.AddEvent.DuplicateKey",
                        "Event %p already exists with key '%s' replacing with new event %p '%s'",
                        oldEvent, oldEvent->GetName().c_str(), emotionEvent, eventName.c_str());
    delete oldEvent;
    
    it->second = emotionEvent;
  }
  else
  {
    _events[eventName] = emotionEvent;
  }
}
  

static const char* kEmotionEventsKey = "emotionEvents";

  
bool EmotionEventMapper::LoadEmotionEvent(const Json::Value& inJson)
{
  EmotionEvent* newEvent = new EmotionEvent();
  if (newEvent->ReadFromJson(inJson))
  {
    AddEvent(newEvent);
    return true;
  }
  else
  {
    delete newEvent;
    return false;
  }
}
  
  
bool EmotionEventMapper::LoadEmotionEvents(const Json::Value& inJson)
{
  const Json::Value& emotionEvents = inJson[kEmotionEventsKey];
  if (emotionEvents.isNull())
  {
    // Might be 1 loose event instead of an array - try loading it
    
    if (inJson.isNull() || !LoadEmotionEvent(inJson))
    {
      PRINT_NAMED_WARNING("EmotionEventMapper.LoadEmotionEvents.MissingValue", "Missing '%s' entry", kEmotionEventsKey);
      return false;
    }
    else
    {
      // Loaded single loose event correctly
      return true;
    }
  }
  
  const uint32_t numEvents = emotionEvents.size();
  
  const Json::Value kNullEventValue;
  
  for (uint32_t i = 0; i < numEvents; ++i)
  {
    const Json::Value& eventJson = emotionEvents.get(i, kNullEventValue);
    
    if (eventJson.isNull() || !LoadEmotionEvent(eventJson))
    {
      PRINT_NAMED_WARNING("EmotionEventMapper.LoadEmotionEvents.BadEvent", "Event %u failed to read", i);
      return false;
    }
  }
  
  return true;
}


bool EmotionEventMapper::ReadFromJson(const Json::Value& inJson)
{
  ClearEvents();
  return LoadEmotionEvents(inJson);
}


bool EmotionEventMapper::WriteToJson(Json::Value& outJson) const
{
  assert( outJson.empty() );
  outJson.clear();
  
  {
    Json::Value emotionEvents(Json::arrayValue);
    
    for (const auto& kv : _events)
    {
      const EmotionEvent* emotionEvent = kv.second;
      
      Json::Value eventJson;
      emotionEvent->WriteToJson(eventJson);
      emotionEvents.append(eventJson);
    }
    
    outJson[kEmotionEventsKey] = emotionEvents;
  }
  
  return true;
}

  
} // namespace Vector
} // namespace Anki

