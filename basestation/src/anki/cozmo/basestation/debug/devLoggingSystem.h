/**
* File: devLoggingSystem
*
* Author: Lee Crippen
* Created: 3/30/2016
*
* Description: 
*
* Copyright: Anki, inc. 2016
*
*/
#ifndef __Basestation_Debug_DevLoggingSystem_H_
#define __Basestation_Debug_DevLoggingSystem_H_

#include "util/helpers/noncopyable.h"

#include <memory>
#include <string>
#include <chrono>

// Forward declarations
namespace Anki {
namespace Util {
  class RollingFileLogger;
}
}

namespace Anki {
namespace Cozmo {

class DevLoggingSystem : Util::noncopyable {
public:
  DevLoggingSystem();
  virtual ~DevLoggingSystem();
  
  template<typename MsgType>
  void LogMessage(const MsgType& message);
  
  static const std::chrono::system_clock::time_point& GetAppRunStartTime() { return kAppRunStartTime; }
  static void SetDevLoggingBaseDirectory(const std::string& directory) { kDevLoggingBaseDirectory = directory; }
  static const std::string& GetDevLoggingBaseDirectory() { return kDevLoggingBaseDirectory; }
  
private:
  std::unique_ptr<Util::RollingFileLogger>    _gameToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToGameLog;
  std::unique_ptr<Util::RollingFileLogger>    _robotToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToRobotLog;
  
  std::chrono::system_clock::time_point       _startTime;
  
  template<typename MsgType>
  std::string PrepareMessage(const MsgType& message);
  
  static const std::chrono::system_clock::time_point kAppRunStartTime;
  
  static std::string kDevLoggingBaseDirectory;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Debug_DevLoggingSystem_H_
