/**
 * File:        sim_DAS.cpp
 *
 * Description: DAS logging functions for vic-robot
 *
 **/

#include "anki/cozmo/robot/DAS.h"

namespace Anki {
namespace Cozmo {
namespace RobotInterface {

void DasLogEvent(const std::string& logLevel, const DasMsg & dasMsg)
{
  printf("[%s] %s: s1: %s, s2: %s, s3: %s, s4: %s, i1: %s, i2: %s, i3: %s, i4: %s\n",
         logLevel.c_str(),
         dasMsg.event.c_str(), 
         dasMsg.s1.c_str(), dasMsg.s2.c_str(), dasMsg.s3.c_str(), dasMsg.s4.c_str(),
         dasMsg.i1.c_str(), dasMsg.i2.c_str(), dasMsg.i3.c_str(), dasMsg.i4.c_str());
}

void sLogError(const DasMsg & dasMessage)
{
  DasLogEvent("DASError", dasMessage);
}

void sLogWarning(const DasMsg & dasMessage)
{
  DasLogEvent("DASWarn", dasMessage);
}

void sLogInfo(const DasMsg & dasMessage)
{
  DasLogEvent("DASInfo", dasMessage);
}

void sLogDebug(const DasMsg & dasMessage)
{
  DasLogEvent("DASDebug", dasMessage);
}

} // namespace RobotInterface
} // namespace Cozmo
} // namespace Anki

