/**
 * File: cladLoggerProvider.cpp
 *
 * Author: Molly Jameson
 * Created: 12/6/16
 *
 * Description:
 *
 * Copyright: Anki, inc. 2016
 *
 */

#include "anki/cozmo/basestation/debug/cladLoggerProvider.h"

#include "anki/cozmo/basestation/externalInterface/externalInterface.h"
#include "anki/cozmo/basestation/robot.h"
#include "clad/externalInterface/messageEngineToGame.h"

namespace Anki {
namespace Cozmo {

void CLADLoggerProvider::Log(Anki::Util::ILoggerProvider::LogLevel logLevel, const std::string& message)
{
  if( _externalInterface != nullptr )
  {
    ExternalInterface::DebugAppendConsoleLogLine sendMsg(message);
    // limited by some low-level SafeMessageBuffer in clad.
    // if we need something this long, check the devlog instead.
    const std::size_t kMaxStrLen = 255;
    if( message.length() > kMaxStrLen )
    {
      sendMsg.line = message.substr(0,kMaxStrLen);
    }
    _externalInterface->Broadcast(ExternalInterface::MessageEngineToGame(std::move(sendMsg)));
  }
}

} // end namespace Cozmo
} // end namespace Anki
