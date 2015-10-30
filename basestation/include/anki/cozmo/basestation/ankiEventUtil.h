/**
 * File: ankiEventUtil.h
 *
 * Author: Lee Crippen
 * Created: 10/30/15
 *
 * Description: Helper for ExternalInterface::MessageGameToEngine handling in your class. Add a public
 
 template<typename T>
 void HandleMessage(const T& msg);
 
 to your header. Then, in class, add specializations for each message type to handle like so:
 
 template<>
 void YourClass::HandleMessage(const MessageType& msg)
 {
 
 }
 
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Cozmo_Basestation_AnkiEventUtil_H__
#define __Cozmo_Basestation_AnkiEventUtil_H__


#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "clad/externalInterface/messageGameToEngine.h"

namespace Anki {
namespace Cozmo {
  
// forward declarations
class IExternalInterface;
template<typename T> class AnkiEvent;
  
class AnkiEventUtil
{
public:
  template <ExternalInterface::MessageGameToEngineTag Tag, typename T, typename H>
  static void SubscribeInternal(IExternalInterface& externalInterface, T thisPointer, H& eventHandlers)
  {
    eventHandlers.push_back(externalInterface.Subscribe(Tag,
      [thisPointer] (const AnkiEvent<ExternalInterface::MessageGameToEngine>& event)
      {
        thisPointer->HandleMessage(event.GetData().Get_<Tag>());
      }));
  }
  
}; // class Event


} // namespace Cozmo
} // namespace Anki

#endif //  __Cozmo_Basestation_AnkiEventUtil_H__