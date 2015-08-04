/**
 * File: eventMgr.h
 *
 * Author: Lee Crippen
 * Created: 07/30/15
 *
 * Description: Manager for events to be used across the engine; responsible for keeping track of events that can
 *              trigger, registered listeners, and dispatching events when they occur.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef ANKI_COZMO_EVENTMGR_H
#define ANKI_COZMO_EVENTMGR_H

#include "event.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "util/signals/simpleSignal.hpp"

#include <string>
#include <map>
#include <vector>
#include <functional>

namespace Anki {
namespace Cozmo {

class EventMgr
{
public:
  using SubscriberFunction = std::function<void(const Event&)>;
  using EventHandlerSignal = Signal::Signal<void(const Event&)>;
  
  inline static EventMgr* GetInstance();
  static void RemoveInstance();
  
  EventMgr(const EventMgr&) = delete;
  EventMgr& operator=(const EventMgr&) = delete;
  
  // Broadcasts a given event to everyone that has subscribed to that event type
  void Broadcast(const Event & event);
  
  // Allows subscribing to events by type with the passed in function
  Signal::SmartHandle Subcribe(Event::EventType type, SubscriberFunction function);
  
private:
  EventMgr() { }
  
  static EventMgr* sInstance;
  
  std::map<Event::EventType, EventHandlerSignal>  _eventHandlerMap;
  
}; // class EventMgr


//
// Inlined Implementations
//
inline EventMgr* EventMgr::GetInstance()
{
  // Lazy init of singleton
  if(nullptr == sInstance)
  {
    sInstance = new EventMgr();
  }
  
  return sInstance;
}


} // namespace Cozmo
} // namespace Anki

#endif //  ANKI_COZMO_EVENTMGR_H