/**
* File: externalInterface
*
* Author: damjan stulic
* Created: 7/18/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#ifndef __Anki_Cozmo_Basestation_ExternalInterface_ExternalInterface_H__
#define __Anki_Cozmo_Basestation_ExternalInterface_ExternalInterface_H__

#include <vector>

namespace Anki {
namespace Cozmo {


//forward declarations
class MessageEngineToGame;
class MessageGameToEngine;


class IExternalInterface {
public:
  virtual void DeliverToGame(const MessageEngineToGame&& message);
  //virtual bool HasMoreMessagesForEngine() const;
  //virtual const MessageGameToEngine GetNextUndeliveredMessage();
  //virtual void GetNextUndeliveredMessages();
};


class ExternalInterface : public IExternalInterface {
public:
  void DeliverToGame(const MessageEngineToGame&& message) override;

};

} // end namespace Cozmo
} // end namespace Anki

#endif //__Anki_Cozmo_Basestation_ExternalInterface_ExternalInterface_H__
