/**
 * File: ankiEventMgr.h
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

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "util/signals/simpleSignal.hpp"

#include <map>
#include <functional>

namespace Anki {
namespace Cozmo {

template<typename DataType>
class AnkiEventMgr
{
public:
  using EventDataType = AnkiEvent<DataType>;
  using SubscriberFunction = std::function<void(const EventDataType&)>;
  using EventHandlerSignal = Signal::Signal<void(const EventDataType&)>;
  
  inline static AnkiEventMgr* GetInstance();
  static void RemoveInstance();
  
  AnkiEventMgr(const AnkiEventMgr&) = delete;
  AnkiEventMgr& operator=(const AnkiEventMgr&) = delete;
  
  // Broadcasts a given event to everyone that has subscribed to that event type
  void Broadcast(const EventDataType& event);
  
  // Allows subscribing to events by type with the passed in function
  Signal::SmartHandle Subcribe(u32 type, SubscriberFunction function);
  
private:
  AnkiEventMgr() { }
  
  static AnkiEventMgr* sInstance;
  
  std::map<u32, EventHandlerSignal>  _eventHandlerMap;
  
}; // class AnkiEventMgr
  
  
// Template definitions included here
#include "ankiEventMgr_templates.def"

} // namespace Cozmo
} // namespace Anki

#endif //  ANKI_COZMO_EVENTMGR_H