/**
* File: messageReceiver
*
* Author: damjan
* Created: 7/24/14
*
* Description: Base class for receiving messages. To use either:
*  - extend from this class
*  - have an instance of this object in your class
*  Once the dispatcher is set, you can subscribe for different
*  message types by using multiple receiver methods. At descruction
*  time there is automatic subscription removal.
*
*
* Copyright: Anki, Inc. 2014
*
**/

#ifndef Util_MessageDispatch_MessageReceiver_H__
#define Util_MessageDispatch_MessageReceiver_H__

#include "util/messageDispatch/messageDispatch_fwd.h"

namespace Anki {
namespace Util {

template <typename MessageType, typename ...Args>
class MessageReceiver {
  friend class MessageDispatcher<MessageType, Args...>;
  
public:
  using MessageType_t = MessageType;
  using MessageDispatcher_t = MessageDispatcher<MessageType, Args...>;
  using MessageResponder_t = MessageResponder<MessageType, Args...>;
  using MessageTag_t = typename MessageType::Tag;
  static_assert(std::is_enum<MessageTag_t>::value,
                "Message Tag must be an enum type");
  
  MessageReceiver<MessageType, Args...>() : _dispatcher(nullptr) {};
  virtual ~MessageReceiver<MessageType, Args...>()
  {
    Unsubscribe();
    _dispatcher = nullptr;
  }
  
  void SetDispatcher(MessageDispatcher_t* dispatcher)
  {
    if (dispatcher != _dispatcher ) {
      Unsubscribe();
      _dispatcher = dispatcher;
      SetDispatcherPrivate(_dispatcher);
    }
  }
  
  void Subscribe(MessageTag_t messageTag, const MessageResponder_t& responder)
  {
    assert(nullptr != _dispatcher);
    if(nullptr != _dispatcher) {
      _dispatcher->Subscribe(this, messageTag, responder);
    }
  }
  
  void Unsubscribe(MessageTag_t messageTag)
  {
    if(nullptr != _dispatcher) {
      _dispatcher->Unsubscribe(this, messageTag);
    }
  }
  
  void Unsubscribe()
  {
    if (nullptr != _dispatcher) {
      _dispatcher->Unsubscribe(this);
    }
  }
  
  void SendMessage(const MessageType_t* message, Args ... args)
  {
    if (nullptr != _dispatcher) {
      _dispatcher->SendMessage(message, args...);
    }
  }

protected:
  MessageDispatcher_t* _dispatcher;
  
private:
  void ClearDispatcher() { _dispatcher = nullptr; }
  virtual void SetDispatcherPrivate(MessageDispatcher_t* dispatcher) {};
};


} // namespace Util
} // namespace Anki


#endif //Util_MessageDispatch_MessageReceiver_H__
