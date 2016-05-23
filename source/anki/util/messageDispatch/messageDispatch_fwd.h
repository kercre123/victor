/**
 * File: messageDispatch_fwd
 *
 * Author: pauley
 * Created: 3/24/15
 *
 * Description: Message Dispatcher delivers messages (along with agreed upon arguments) to subscribers.
 * Delivers each message to all subscribed pairs before message is deleted.
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef UTIL_MESSAGEDISPATCH_FWD_H__
#define UTIL_MESSAGEDISPATCH_FWD_H__


#include <functional>

namespace Anki {
namespace Util {

template <class MessageType, typename ...Args>
using MessageResponder = std::function<void (const MessageType&, Args...)>;
  
template <typename MessageType, typename ... Args>
class MessageReceiver;

template <typename MessageType, typename ... Args>
class MessageDispatcher;
  

} // namespace DriveEngine
} // namespace Anki


#endif