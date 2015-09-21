/**
 * File: ankiMailboxEventMgr.h
 *
 * Author: Damjan Stulic
 * Created: 09/09/15
 *
 * Description: Manager for events to be used across the engine; responsible for keeping track of events that can
 *              trigger, registered listeners, and dispatching events when they occur. subscription and broadcasting
 *              takes in another param: mailbox id. Mailbox allows us to listen for events coming from specific
 *              device, or going to specific device. For example, if you want to subscribe to "battery_status" message
 *              coming only from device "3".
 *
 * Copyright: Anki, Inc. 2015
 *
 * COZMO_PUBLIC_HEADER
 **/

#ifndef __Anki_Cozmo_Basestation_Events_AnkiMailboxEventMgr_H__
#define __Anki_Cozmo_Basestation_Events_AnkiMailboxEventMgr_H__

#pragma mark

#include "anki/cozmo/basestation/events/ankiEvent.h"
#include "util/signals/simpleSignal.hpp"
#include "util/helpers/noncopyable.h"
#include <unordered_map>
#include <functional>

namespace Anki {
namespace Cozmo {


template<typename DataType>
class AnkiMailboxEventMgr : private Util::noncopyable
{
public:
  const uint32_t AnyMailboxId{65999};
  using EventDataType = AnkiEvent<DataType>;
  using SubscriberFunction = std::function<void(const EventDataType&)>;
  using EventHandlerSignal = Signal::Signal<void(const EventDataType&)>;

  // Broadcasts a given event to everyone that has subscribed to that event type
  void Broadcast(const uint32_t mailbox, const EventDataType& event);

  // Allows subscribing to events by type with the passed in function
  Signal::SmartHandle Subcribe(const uint32_t mailbox, const uint32_t type, SubscriberFunction function);
  void SubscribeForever(const uint32_t mailbox, const uint32_t type, SubscriberFunction function);
  void UnsubscribeAll();

private:
  std::unordered_map<uint32_t, std::unordered_map<uint32_t, EventHandlerSignal > >  _eventHandlerMap;
}; // class AnkiEventMgr


// Template definitions included here
template <typename DataType>
void AnkiMailboxEventMgr<DataType>::Broadcast(const uint32_t mailbox, const EventDataType& event)
{
  auto iter = _eventHandlerMap.find(event.GetType());
  if (iter != _eventHandlerMap.end())
  {
    if (mailbox == AnyMailboxId)
    {
      // deliver to all mailboxes
      for (auto& mapPair : iter->second) {
        mapPair.second.emit(event);
      }
    } else {
      // iter->second is a map: std::map<uint32_t, EventHandlerSignal >
      // search the inner map for correct mailbox
      auto innerIter = iter->second.find(mailbox);
      if (innerIter != iter->second.end()) {
        innerIter->second.emit(event);
      }
      // also search for AnyMailbox
      innerIter = iter->second.find(AnyMailboxId);
      if (innerIter != iter->second.end()) {
        innerIter->second.emit(event);
      }
    }
  }
}

template <typename DataType>
Signal::SmartHandle AnkiMailboxEventMgr<DataType>::Subcribe(const uint32_t mailbox, const uint32_t type, SubscriberFunction function)
{
  return _eventHandlerMap[type][mailbox].ScopedSubscribe(function);
}

template <typename DataType>
void AnkiMailboxEventMgr<DataType>::SubscribeForever(const uint32_t mailbox, const uint32_t type, SubscriberFunction function)
{
  _eventHandlerMap[type][mailbox].SubscribeForever(function);
}

template <typename DataType>
void AnkiMailboxEventMgr<DataType>::UnsubscribeAll()
{
  _eventHandlerMap.clear();
}

} // namespace Cozmo
} // namespace Anki

#endif //  __Anki_Cozmo_Basestation_Events_AnkiMailboxEventMgr_H__