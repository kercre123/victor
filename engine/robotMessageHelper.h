/**
 * File: robotMessageHelper.h
 *
 * Author: Brad Neuman
 * Created: 2019-03-24
 *
 * Description: Helper to send robot messages with profiling and cleaner syntax
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#ifndef __Engine_RobotMessageHelper_H__
#define __Engine_RobotMessageHelper_H__

#include "coretech/common/shared/types.h"

#include <utility>

namespace Anki {
namespace Vector {

namespace RobotInterface {
class MessageHandler;
class EngineToRobot;
}

namespace RobotMessageHelper {

// Send a message to the physical robot
Result SendMessage(RobotInterface::MessageHandler* messageHandler,
                   const RobotInterface::EngineToRobot& message,
                   bool reliable = true,
                   bool hot = false);

// Helper template for sending Robot messages with clean syntax
template<typename T, typename... Args>
Result SendRobotMessage(RobotInterface::MessageHandler* messageHandler, Args&&... args)
{
  return SendMessage(messageHandler, RobotInterface::EngineToRobot(T(std::forward<Args>(args)...)));
}


}
}
}

#endif
