/**
* File: devLoggerProvider
*
* Author: Lee Crippen
* Created: 6/21/2016
*
* Description: Extension of the SaveToFileLoggerProvider to add milliseconds since app start to the front of messages
*
* Copyright: Anki, inc. 2016
*
*/

#include "anki/cozmo/basestation/debug/devLoggerProvider.h"
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"

#include <sstream>
#include <iomanip>

namespace Anki {
namespace Cozmo {
  
  
DevLoggerProvider::DevLoggerProvider(Util::Dispatch::Queue* queue, const std::string& baseDirectory, std::size_t maxFileSize)
: Util::SaveToFileLoggerProvider(queue, baseDirectory, maxFileSize)
{
}
  
void DevLoggerProvider::Log(ILoggerProvider::LogLevel logLevel, const std::string& message)
{
  size_t messageSize = message.size();
  
  std::ostringstream modMessageStream;
  modMessageStream << std::setfill('0') << std::setw(7) << DevLoggingSystem::GetAppRunMilliseconds() << " " << messageSize << " " << message;
  Util::SaveToFileLoggerProvider::Log(logLevel, modMessageStream.str());
}

} // end namespace Cozmo
} // end namespace Anki
