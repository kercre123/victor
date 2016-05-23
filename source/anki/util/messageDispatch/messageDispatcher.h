/**
* File: messageDispatcher
*
* Author: damjan
* Created: 7/24/14
*
* Description: Keeps a map of all receivers,responder pairs.
* Delivers each message to all subscribed pairs. before message is deleted.
*
*
* Copyright: Anki, Inc. 2014
*
**/

#ifndef Util_MessageDispatch_MessageDispatcher_H__
#define Util_MessageDispatch_MessageDispatcher_H__

#include "util/messageDispatch/messageDispatch_fwd.h"

#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Anki {
namespace Util {

template <class MessageType, typename ...Args>
class MessageDispatcher {
  
public:
  
  // The message type.
  using MessageType_t = MessageType;
  // The tag for this message.
  using MessageTag_t = typename MessageType_t::Tag;
  static_assert(std::is_enum<MessageTag_t>::value,
                "Message Tag must be an enum type");

  
  using MessageResponder_t = MessageResponder<MessageType, Args...>;
  using MessageReceiver_t = MessageReceiver<MessageType, Args...>;
  
  virtual ~MessageDispatcher()
  {
    RemoveAllSubscribers();
  }
  
  void Subscribe(MessageReceiver_t* receiver, MessageTag_t playerMessageType, const MessageResponder_t& responder)
  {
    assert(receiver != nullptr);
    auto mapIt = _receivers.find(playerMessageType);
    if (mapIt != _receivers.end()) {
      mapIt->second.emplace_back(receiver, responder);
    }
    else {
      std::vector<ReceiverTuple> receivers;
      receivers.emplace_back(receiver, responder);
      _receivers.emplace(playerMessageType, std::move(receivers));
    }
  }
  
  void Unsubscribe(const MessageReceiver_t* receiver, MessageTag_t messageTag)
  {
    assert(receiver != nullptr);
    auto foundIt = _receivers.find(messageTag);
    if (foundIt != _receivers.end()) {
      foundIt->second.erase(std::remove_if(foundIt->second.begin(), foundIt->second.end(),
                                           [receiver](ReceiverTuple& tuple) { return tuple.receiver == receiver; }),
                            foundIt->second.end());
    }
  }
  
  void Unsubscribe(const MessageReceiver_t* receiver)
  {
    assert(receiver != nullptr);
    for (auto& pair : _receivers) {
      pair.second.erase(std::remove_if(pair.second.begin(), pair.second.end(),
                                       [receiver](ReceiverTuple& tuple) { return tuple.receiver == receiver; }),
                        pair.second.end());
    }
  }
  
  virtual void SendMessage(const MessageType_t* message, Args...) = 0;
  
protected:

  void DispatchMessage(const MessageType& message, Args ... args)
  {
    auto mapIt = _receivers.find(message.GetTag());
    if (mapIt != _receivers.end()) {
      for (const auto& tuple : mapIt->second) {
        tuple.responder(message, args...);
      }
    }
  }
public:
  void RemoveAllSubscribers()
  {
    std::unordered_set<MessageReceiver_t*> receivers;
    for (const auto& pair : _receivers) {
      for (auto& tuple : pair.second) {
        tuple.receiver->ClearDispatcher();
      }
    }
    _receivers.clear();
  }
  
private:
  struct ReceiverTuple {
    ReceiverTuple(MessageReceiver_t* receiver, const MessageResponder_t& responder)
      : receiver(receiver), responder(responder) {};
    
    MessageReceiver_t* receiver;
    MessageResponder_t responder;
  };
  std::unordered_map<MessageTag_t, std::vector<ReceiverTuple> > _receivers;
};

} // namespace Util
} // namespace Anki


#endif //Util_MessageDispatch_MessageDispatcher_H__
