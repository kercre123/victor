/**
* File: externalInterface
*
* Author: damjan stulic
* Created: 7/18/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
* COZMO_PUBLIC_HEADER
*/

#ifndef __Anki_Cozmo_Basestation_ExternalInterface_ExternalInterface_H__
#define __Anki_Cozmo_Basestation_ExternalInterface_ExternalInterface_H__

#include "anki/cozmo/basestation/events/ankiEventMgr.h"

#include <vector>

namespace Anki {
namespace Cozmo {

//forward declarations
namespace ExternalInterface {
class MessageEngineToGame;
class MessageGameToEngine;
} // end namespace ExternalInterface


class IExternalInterface {
public:
  virtual ~IExternalInterface() {};
  
  virtual void Broadcast(const ExternalInterface::MessageGameToEngine& message) = 0;
  virtual void Broadcast(ExternalInterface::MessageGameToEngine&& message) = 0;
  
  virtual void Broadcast(const ExternalInterface::MessageEngineToGame& message) = 0;
  virtual void Broadcast(ExternalInterface::MessageEngineToGame&& message) = 0;
  
protected:
  virtual void DeliverToGame(const ExternalInterface::MessageEngineToGame& message) = 0;
  
  //virtual bool HasMoreMessagesForEngine() const;
  //virtual const MessageGameToEngine GetNextUndeliveredMessage();
  //virtual void GetNextUndeliveredMessages();
};


class SimpleExternalInterface : public IExternalInterface {
protected:
  void DeliverToGame(const ExternalInterface::MessageEngineToGame& message) override;

};

} // end namespace Cozmo
} // end namespace Anki

#endif //__Anki_Cozmo_Basestation_ExternalInterface_ExternalInterface_H__
