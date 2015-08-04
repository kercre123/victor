/**
 * File: event.h
 *
 * Author: Lee Crippen
 * Created: 07/30/15
 *
 * Description: Events that contain a map of names to anonymous data. Each piece of data has a type
 *              and a unionized value.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef ANKI_COZMO_EVENT_H
#define ANKI_COZMO_EVENT_H

#include "anki/cozmo/shared/cozmoTypes.h"

#include "assert.h"
#include <string>
#include <map>


namespace Anki {
namespace Cozmo {

class Event
{
public:
  enum class EventType : u32
  {
    EVENT_UIDEVICE_CONNECTED,
  };
  
  enum class EventDataType
  {
    BOOLEAN,
    U_8,
    S_8,
    U_16,
    S_16,
    U_32,
    S_32,
    U_64,
    S_64,
    F_32,
    F_64,
  };
  
  Event(EventType type) : _myType(type) { }
  
  EventType GetType() const { return _myType; }
  
  Result AddData(std::string name, EventDataType type, const void * valuePointer);
  
  template <typename RetTYPE>
  RetTYPE GetData(std::string name, EventDataType type) const;
  
protected:
  
  struct EventData
  {
    EventDataType type;
    union {
      bool boolean;
      u8  u8;
      s8  s8;
      u16 u16;
      s16 s16;
      u32 u32;
      s32 s32;
      u64 u64;
      s64 s64;
      f32 f32;
      f64 f64;
    };
  };
  
  std::map<std::string, EventData> _eventData;
  EventType _myType;
  
}; // class Event

template <typename RetTYPE>
RetTYPE Event::GetData(std::string name, EventDataType type) const
{
  switch (type) {
    case EventDataType::BOOLEAN:
    {
      return static_cast<RetTYPE>(_eventData.at(name).boolean);
    }
    case EventDataType::U_8:
    {
      return static_cast<RetTYPE>(_eventData.at(name).u8);
      break;
    }
    case EventDataType::S_8:
    {
      return static_cast<RetTYPE>(_eventData.at(name).s8);
      break;
    }
    case EventDataType::U_16:
    {
      return static_cast<RetTYPE>(_eventData.at(name).u16);
      break;
    }
    case EventDataType::S_16:
    {
      return static_cast<RetTYPE>(_eventData.at(name).s16);
      break;
    }
    case EventDataType::U_32:
    {
      return static_cast<RetTYPE>(_eventData.at(name).u32);
      break;
    }
    case EventDataType::S_32:
    {
      return static_cast<RetTYPE>(_eventData.at(name).s32);
      break;
    }
    case EventDataType::U_64:
    {
      return static_cast<RetTYPE>(_eventData.at(name).u64);
      break;
    }
    case EventDataType::S_64:
    {
      return static_cast<RetTYPE>(_eventData.at(name).s64);
      break;
    }
    case EventDataType::F_32:
    {
      return static_cast<RetTYPE>(_eventData.at(name).f32);
      break;
    }
    case EventDataType::F_64:
    {
      return static_cast<RetTYPE>(_eventData.at(name).f64);
      break;
    }
    default:
    {
      assert(false);
      return static_cast<RetTYPE>(0);
    }
  }
}

} // namespace Cozmo
} // namespace Anki

#endif //  ANKI_COZMO_EVENT_H