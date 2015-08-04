#include "anki/cozmo/basestation/events/eventMgr.h"

#include "anki/common/types.h"

namespace Anki {
namespace Cozmo {

EventMgr* EventMgr::sInstance = nullptr;

void EventMgr::RemoveInstance()
{
  if (nullptr != sInstance)
  {
    delete sInstance;
    sInstance = nullptr;
  }
}

void EventMgr::Broadcast(const Event& event)
{
  Event::EventType type = event.GetType();
  _eventHandlerMap[type].emit(event);
}

Signal::SmartHandle EventMgr::Subcribe(Event::EventType type, SubscriberFunction function)
{
  return _eventHandlerMap[type].ScopedSubscribe(function);
}

} // namespace Cozmo
} // namespace Anki