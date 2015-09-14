/**
* File: messageHandlerStub
*
* Author: damjan stulic
* Created: 9/8/15
*
* Description:
*
* Copyright: Anki, inc. 2015
*
*/

#ifndef __Anki_Cozmo_Basestation_RobotInterface_MessageHandlerStub_H__
#define __Anki_Cozmo_Basestation_RobotInterface_MessageHandlerStub_H__

#include "anki/cozmo/basestation/robotInterface/messageHandler.h"

namespace Anki {

namespace Comms {
class IChannel;
}

namespace Cozmo {
namespace RobotInterface {

class EngineToRobot;
class RobotToEngine;
enum class EngineToRobotTag : uint8_t;
enum class RobotToEngineTag : uint8_t;


class MessageHandlerStub : public IMessageHandler
{
public:
  MessageHandlerStub() { }
  Result Init(Comms::IChannel* comms, RobotManager* robotMgr) override { return RESULT_OK; }
  Result ProcessMessages() override { return RESULT_OK; }
  Result SendMessage(const RobotID_t robotID, const RobotInterface::EngineToRobot& msg, bool reliable = true, bool hot = false) override
  {
    return RESULT_OK;
  }
};


} // end namespace RobotInterface
} // end namespace Cozmo
} // end namespace Anki



#endif //__Anki_Cozmo_Basestation_RobotInterface_MessageHandlerStub_H__
