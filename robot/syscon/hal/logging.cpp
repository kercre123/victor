#include <stdarg.h>
#include "messages.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"
#include "anki/cozmo/robot/logging.h"

static u16 missedLogs;

void ResetMissedLogCount()
{
  missedLogs = 0;
}

int Anki::Cozmo::RobotInterface::SendLog(const LogLevel level, const uint16_t name, const uint16_t formatId, const uint8_t numArgs, ...)
{
  using namespace Anki::Cozmo::RobotInterface;
  PrintTrace m;
  if (missedLogs > 0)
  {
    m.level = ANKI_LOG_LEVEL_EVENT;
    m.name  = 3;
    m.stringId = 1;
    m.value_length = 1;
    m.value[0] = missedLogs + 1; // +1 for the message we are dropping in thie call to SendLog
  }
  else
  {
    m.level = level;
    m.name  = name;
    m.stringId = formatId;
    va_list argptr;
    va_start(argptr, numArgs);
    for (m.value_length=0; m.value_length < numArgs; m.value_length++)
    {
      m.value[m.value_length] = va_arg(argptr, int);
    }
    va_end(argptr);
  }
  if (SendMessage(m))
  {
    ResetMissedLogCount();
  }
  else
  {
    missedLogs++;
  }
  return 0;
}
