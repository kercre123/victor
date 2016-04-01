#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "util/logging/rollingFileLogger.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"

#include <sstream>

namespace Anki {
namespace Cozmo {

DevLoggingSystem::DevLoggingSystem(const std::string& baseDirectory)
  : _startTime(std::chrono::system_clock::now())
{
  auto getPathString = [baseDirectory] (const std::string& path) -> std::string
  {
    std::ostringstream pathStream;
    if (!baseDirectory.empty())
    {
      pathStream << baseDirectory << '/';
    }
    
    pathStream << path;
    return pathStream.str();
  };
  
  _gameToEngineLog.reset(new Util::RollingFileLogger(getPathString("gameToEngine")));
  _engineToGameLog.reset(new Util::RollingFileLogger(getPathString("engineToGame")));
  _robotToEngineLog.reset(new Util::RollingFileLogger(getPathString("robotToEngine")));
  _engineToRobotLog.reset(new Util::RollingFileLogger(getPathString("engineToRobot")));
}

DevLoggingSystem::~DevLoggingSystem() = default;

template<typename MsgType>
std::string DevLoggingSystem::PrepareMessage(const MsgType& message)
{
  static const std::size_t extraSize = sizeof(uint32_t) * 2;
  const std::size_t messageSize = message.Size();
  const std::size_t totalSize = messageSize + extraSize;
  std::vector<uint8_t> messageVector(totalSize);
  
  uint8_t* data = messageVector.data();
  
  *((uint32_t*)data) = static_cast<uint32_t>(totalSize);
  data += sizeof(uint32_t);
  
  auto timeNow = std::chrono::system_clock::now();
  auto msElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - _startTime);
  *((uint32_t*)data) = static_cast<uint32_t>(msElapsed.count());
  data += sizeof(uint32_t);
  
  message.Pack(data, messageSize);
  
  return std::string(reinterpret_cast<char*>(messageVector.data()), totalSize);
}

template<>
void DevLoggingSystem::LogMessage(const ExternalInterface::MessageEngineToGame& message)
{
  _engineToGameLog->Write(PrepareMessage(message));
}
  
template<>
void DevLoggingSystem::LogMessage(const ExternalInterface::MessageGameToEngine& message)
{
  _gameToEngineLog->Write(PrepareMessage(message));
}

template<>
void DevLoggingSystem::LogMessage(const RobotInterface::EngineToRobot& message)
{
  _engineToRobotLog->Write(PrepareMessage(message));
}

template<>
void DevLoggingSystem::LogMessage(const RobotInterface::RobotToEngine& message)
{
  static const int logEveryNth = 10;
  static int imageCount = logEveryNth;
  
  if (RobotInterface::RobotToEngineTag::image == message.GetTag())
  {
    --imageCount;
    if (imageCount > 0)
    {
      // Bail early if we haven't gotten to the image message we want
      return;
    }
    
    // This is an image we're going to log so reset the counter
    imageCount = logEveryNth;
  }
  
  _robotToEngineLog->Write(PrepareMessage(message));
}

} // end namespace Cozmo
} // end namespace Anki
