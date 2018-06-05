/**
* File: externalMessageRouter
*
* Author: ross
* Created: may 31 2018
*
* Description: Templates to automatically wrap messages included in the MessageEngineToGame union
*              based on external requirements (the hierarchy in MessageEngineToGame clad).
*
* Copyright: Anki, Inc. 2018
*/

#ifndef __Engine_ExternalInterface_ExternalMessageRouter_H__
#define __Engine_ExternalInterface_ExternalMessageRouter_H__

#include "clad/externalInterface/messageEngineToGame.h"
#include "util/helpers/templateHelpers.h"
#include <type_traits>

namespace Anki {
namespace Cozmo {

class ExternalMessageRouter
{
public:
  
  // the hierarchy, based on messageEngineToGame.clad:
  using MessageEngineToGame = ExternalInterface::MessageEngineToGame;
  // which contains
  using Event = ExternalInterface::Event;
  // which contains
  using Status = ExternalInterface::Status;
  
  // helper that provides type Ret if B is true
  template <bool B, class Ret>
  using ReturnIf = typename std::enable_if<B, Ret>::type;
  
  // Is an Event (not part of a request/response pair)
  template <typename T>
  using CanBeEvent = Anki::Util::is_explicitly_constructible<Event, T>;
  
  // Is a sub-status message (which is an Event)
  template <typename T>
  using CanBeStatus = Anki::Util::is_explicitly_constructible<Status, T>;
  
  template <typename T>
  using CanBeEngineToGame = Anki::Util::is_explicitly_constructible<MessageEngineToGame, T>;
  
  
  // in case this is used with a MessageEngineToGame
  inline static MessageEngineToGame Wrap(MessageEngineToGame&& message)
  {
    return message;
  }
  
  template <typename T>
  inline static ReturnIf<!CanBeStatus<T>::value && !CanBeEvent<T>::value, MessageEngineToGame>
  /* MessageEngineToGame */ Wrap(T&& message)
  {
    MessageEngineToGame engineToGame{ std::forward<T>(message) };
    return engineToGame;
  }
  
  template <typename T>
  inline static ReturnIf<CanBeEvent<T>::value, MessageEngineToGame>
  /* MessageEngineToGame */ Wrap(T&& message)
  {
    Event event{ std::forward<T>(message) };
    return Wrap( std::move(event) );
  }

  template <typename T>
  inline static ReturnIf<CanBeStatus<T>::value, MessageEngineToGame>
  /* MessageEngineToGame */ Wrap(T&& message)
  {
    Status status{ std::forward<T>(message) };
    return Wrap( std::move(status) );
  }
  
};


} // end namespace Cozmo
} // end namespace Anki

#endif //__Engine_ExternalInterface_ExternalMessageRouter_H__
