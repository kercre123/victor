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
 * COZMO_PUBLIC_HEADER
 **/

#ifndef ANKI_COZMO_EVENTMGR_H
#define ANKI_COZMO_EVENTMGR_H

#pragma mark

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "anki/cozmo/shared/cozmoTypes.h"
#include "util/signals/simpleSignal.hpp"
#include "util/helpers/noncopyable.h"

#include <map>
#include <functional>

namespace Anki {
namespace Cozmo {

template<typename DataType>
class AnkiEventMgr : private Util::noncopyable
{
public:
  using EventDataType = AnkiEvent<DataType>;
  using SubscriberFunction = std::function<void(const EventDataType&)>;
  using EventHandlerSignal = Signal::Signal<void(const EventDataType&)>;
  
  // Broadcasts a given event to everyone that has subscribed to that event type
  void Broadcast(const EventDataType& event);
  
  // Allows subscribing to events by type with the passed in function
  Signal::SmartHandle Subcribe(u32 type, SubscriberFunction function);
  
private:
  std::map<u32, EventHandlerSignal>  _eventHandlerMap;
  
}; // class AnkiEventMgr
  
  
// Template definitions included here
template <typename DataType>
void AnkiEventMgr<DataType>::Broadcast(const EventDataType& event)
{
  typename std::map<u32, EventHandlerSignal>::iterator iter = _eventHandlerMap.find(event.GetType());
  if (iter != _eventHandlerMap.end())
  {
    iter->second.emit(event);
  }
}
  
template <typename DataType>
Signal::SmartHandle AnkiEventMgr<DataType>::Subcribe(u32 type, SubscriberFunction function)
{
  return _eventHandlerMap[type].ScopedSubscribe(function);
}

} // namespace Cozmo
} // namespace Anki

#endif //  ANKI_COZMO_EVENTMGR_H