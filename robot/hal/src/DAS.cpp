/**
 * File:        DAS.cpp
 *
 * Description: DAS logging functions for vic-robot
 *
 **/

#include "anki/cozmo/robot/DAS.h"
#include <android/log.h>

namespace Anki {
namespace Vector {
namespace RobotInterface {

// Basically a copy of VictorLogger::LogEvent() in Util
void DasLogEvent(int prio, const DasMsg & dasMsg)
{
  //
  // Marshal values for each DAS v2 event fields that must be provided.
  // Note some fields will be provided by the log record itself,
  // while others will be provided by the aggregator.
  //
  //const char * SOURCE = ""; (represented by android log tag)
  //const char * TS = ""; (represented by android log timestamp)
  //const char * LEVEL = ""; (represented by android log level)
  //const char * ROBOT = ""; (provided by aggregator)
  //const char * ROBOT_VERSION = ""; (provided by aggregator)
  //const char * SEQ = seq; (provided by aggregator)
  //const char * PROFILE_ID = ""; (provided by aggregator)
  //const char * FEATURE_TYPE = ""; (provided by aggregator)
  //const char * FEATURE_RUN_ID = ""; (provided by aggregator)
  
  // Format fields into a compact CSV format.
  // Leading @ serves as a hint that this row is in compact CSV format.
  //
  // We use a fixed format string for performance, but we need to update the format string
  // if event format ever changes.
  static_assert(Anki::Vector::RobotInterface::DAS::EVENT_MARKER == '@', "DAS event marker does not match declarations");
  static_assert(Anki::Vector::RobotInterface::DAS::FIELD_MARKER == '\x1F', "DAS field marker does not match declarations");
  static_assert(Anki::Vector::RobotInterface::DAS::FIELD_COUNT == 9, "DAS field count does not match declarations");
  
  __android_log_print(prio, "vic-robot", "@%s\x1F%s\x1F%s\x1F%s\x1F%s\x1F%s\x1F%s\x1F%s\x1F%s",
                      dasMsg.event.c_str(), 
                      dasMsg.s1.c_str(), dasMsg.s2.c_str(), dasMsg.s3.c_str(), dasMsg.s4.c_str(),
                      dasMsg.i1.c_str(), dasMsg.i2.c_str(), dasMsg.i3.c_str(), dasMsg.i4.c_str());
}


void sLogError(const DasMsg & dasMessage)
{
  DasLogEvent(ANDROID_LOG_ERROR, dasMessage);
}

void sLogWarning(const DasMsg & dasMessage)
{
  DasLogEvent(ANDROID_LOG_WARN, dasMessage);
}

void sLogInfo(const DasMsg & dasMessage)
{
  DasLogEvent(ANDROID_LOG_INFO, dasMessage);
}

void sLogDebug(const DasMsg & dasMessage)
{
  DasLogEvent(ANDROID_LOG_DEBUG, dasMessage);
}

} // namespace RobotInterface
} // namespace Vector
} // namespace Anki

