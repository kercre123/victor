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

  //////////////////////////////////////////////////////////
  // Templated subscription functions for forwarding message
  // contents directly to methods
  //
  // Subscribe by passing: message tag, and a handler that will
  // receive the unpacked message contents.
  //
  // The signature of the handler function must match the contents
  // of the CLAD message, in the correct order.
  //
  // Examples:
  //
  //////////////////////////////////
  // GameService: AddAI message
  //////////////////////////////////
  // message AddAI {
  //   uint_32 commanderId,
  //   uint_64 vehicleConnectionUUID
  // }
  //
  // void GameService::AddAI(CommanderDefinitionIdType commanderId,
  //   VehicleConnectionUUID vehicleConnectionUUID); // the commander/vehicle typedefs resolve to uint32/uint64
  //
  // SubscribeMember<ToEngineMessage::Tag::addAI>(&GameService::AddAI);
  //
  //////////////////////////////////
  // PlayerService: RequestConnectToPlayer message
  //////////////////////////////////
  // message PlayerId {
  //   uint_8 playerId,
  // }
  //
  // PlayerService::PlayerId requestConnectToPlayer, // in ToEngineMessage
  //
  // void PlayerService::RequestConnectToPlayer(GamePlayerId gamePlayerId);
  //
  // _externalInterfaceMessageReceiver.SubscribeMember<ToEngineMessage::Tag::requestConnectToPlayer>(
  //   &PlayerService::RequestConnectToPlayer, this);
  //
  // This example has an extra parameter on the end of the SubscribeMember call ("this") to tell
  // the message receiver what object the message handler function (PlayerService::RequestConnectToPlayer)
  // should be invoked on, since PlayerService isn't calling this method on itself, but rather on an external
  // message receiver.
  //
  //////////////////////////////////
  // StoreService: RequestStore message
  //////////////////////////////////
  //
  // Works on void messages/functions too:
  //
  // message RequestStore {
  // }
  //
  // void StoreService::RequestStoreCatalog();
  //
  // SubscribeMember<ToEngineMessage::Tag::requestStore>(&StoreService::RequestStoreCatalog);
  //
  //////////////////////////////////////////////////////////
protected:
  // Protected SubscribeMember functions allow for subscribing
  // member methods of derived classes
  template <MessageTag_t Tag, typename Subscriber, typename ...MsgArgs>
  void SubscribeMember(void (Subscriber::*handlerFunc)(MsgArgs...))
  {
    SubscribeMember<Tag>(handlerFunc, static_cast<Subscriber*>(this));
  }

  template <MessageTag_t Tag, typename Subscriber, typename ...MsgArgs>
  void SubscribeMember(void (Subscriber::*handlerFunc)(MsgArgs...) const)
  {
    SubscribeMember<Tag>(handlerFunc, static_cast<Subscriber*>(this));
  }

public:
  // Public SubscribeMember functions allow for external objects
  // with access to a MessageReceiver to subscribe themselves to
  // a message by passing their instance pointer
  template <MessageTag_t Tag, typename Subscriber, typename ...MsgArgs>
  void SubscribeMember(void (Subscriber::*handlerFunc)(MsgArgs...),
                       Subscriber* subscriber)
  {
    auto boundHandler = [subscriber, handlerFunc] (MsgArgs... args) {
      (subscriber->*handlerFunc)(args...);
    };
    SubscribeCallable<Tag>(boundHandler);
  }

  template <MessageTag_t Tag, typename Subscriber, typename ...MsgArgs>
  void SubscribeMember(void (Subscriber::*handlerFunc)(MsgArgs...) const,
                       Subscriber* subscriber)
  {
    auto boundHandler = [subscriber, handlerFunc] (MsgArgs... args) {
      (subscriber->*handlerFunc)(args...);
    };
    SubscribeCallable<Tag>(boundHandler);
  }

  // SubscribeCallable allows for subscription of any callable object that
  // takes in the parameters contained by the given message type
  template <MessageTag_t Tag, typename Callable>
  void SubscribeCallable(Callable&& messageHandler)
  {
    Subscribe(Tag, [messageHandler] (const MessageType& message, Args...) {
      message.template Get_<Tag>().Invoke(messageHandler);
    });
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
