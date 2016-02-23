#include <stdarg.h>
#include "clad/robotInterface/messageRobotToEngine.h"
#include "clad/robotInterface/messageRobotToEngine_send_helper.h"

int Anki::Cozmo::RobotInterface::SendLog(const LogLevel level, const uint16_t name, const uint16_t formatId, const uint8_t numArgs, ...)
{
  using namespace Anki::Cozmo::RobotInterface;
  static u32 missedMessages = 0;
  PrintTrace m;
  if (missedMessages > 0)
  {
    m.level = ANKI_LOG_LEVEL_WARN;
    m.name  = 1;
    m.stringId = 1;
    m.value_length = 1;
    m.value[0] = missedMessages + 1;
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
    missedMessages = 0;
  }
  else
  {
    missedMessages++;
  }
  return 0;
}
