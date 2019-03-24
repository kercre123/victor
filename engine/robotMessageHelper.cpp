/**
 * File: robotMessageHelper.cpp
 *
 * Author: Brad Neuman
 * Created: 2019-03-24
 *
 * Description: Helper to send robot messages with profiling and cleaner syntax
 *
 * Copyright: Anki, Inc. 2019
 *
 **/

#include "engine/robotMessageHelper.h"

#include "engine/robotInterface/messageHandler.h"
#include "util/logging/logging.h"
#include "util/messageProfiler/messageProfiler.h"

#define LOG_CHANNEL "Robot"

namespace Anki {
namespace Vector {

namespace RobotMessageHelper {

// Send a message to the physical robot
Result SendMessage(RobotInterface::MessageHandler* messageHandler,
                   const RobotInterface::EngineToRobot& msg,
                   bool reliable,
                   bool hot)
{
  static Util::MessageProfiler msgProfiler("RobotMessageHelper::SendMessage");

  Result sendResult = messageHandler->SendMessage(msg, reliable, hot);
  if (sendResult == RESULT_OK) {
    msgProfiler.Update((int)msg.GetTag(), msg.Size());
  } else {
    const char* msgTypeName = EngineToRobotTagToString(msg.GetTag());
    LOG_WARNING("RobotMessageHelper.SendMessage", "Failed to send a message type %s", msgTypeName);
    msgProfiler.ReportOnFailure();
  }
  return sendResult;
}


}
}
}
