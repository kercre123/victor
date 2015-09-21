/**
* File: messageHandler
*
* Author: damjan stulic
* Created: 9/8/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#ifndef __Anki_Cozmo_Basestation_RobotInterface_MessageHandler_H__
#define __Anki_Cozmo_Basestation_RobotInterface_MessageHandler_H__

#include "anki/cozmo/basestation/events/ankiMailboxEventMgr.h"
#include "anki/common/types.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

namespace Anki {

namespace Comms {
class IChannel;
struct IncomingPacket;
}

namespace Cozmo {

class RobotManager;

namespace RobotInterface {

class MessageHandler {
public:

  MessageHandler();
  virtual ~MessageHandler(){};

  virtual void Init(Comms::IChannel* channel, RobotManager* robotMgr);

  virtual void ProcessMessages();

  virtual Result SendMessage(const RobotID_t robotId, const RobotInterface::EngineToRobot& msg, bool reliable = true, bool hot = false);

  Signal::SmartHandle Subscribe(const uint32_t robotId, const RobotInterface::RobotToEngineTag& tagType, std::function<void(const AnkiEvent<RobotInterface::RobotToEngine>&)> messageHandler) {
    return _eventMgr.Subcribe(robotId, static_cast<uint32_t>(tagType), messageHandler);
  }

protected:
  void Broadcast(const uint32_t robotId, const RobotInterface::RobotToEngine& message);
  void Broadcast(const uint32_t robotId, RobotInterface::RobotToEngine&& message);
private:
  void ProcessPacket(const Comms::IncomingPacket& packet);

  AnkiMailboxEventMgr<RobotInterface::RobotToEngine> _eventMgr;
  Comms::IChannel* _channel;
  RobotManager* _robotManager;
  bool _isInitialized;


};


} // end namespace RobotInterface
} // end namespace Cozmo
} // end namespace Anki



#endif //__Anki_Cozmo_Basestation_RobotInterface_MessageHandler_H__
