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
  DevLoggingSystem(const std::string& baseDirectory);
  virtual ~DevLoggingSystem();
  
  template<typename MsgType>
  void LogMessage(const MsgType& message);
  
private:
  std::unique_ptr<Util::RollingFileLogger>    _gameToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToGameLog;
  std::unique_ptr<Util::RollingFileLogger>    _robotToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToRobotLog;
  
  std::chrono::system_clock::time_point       _startTime;
  
  template<typename MsgType>
  std::string PrepareMessage(const MsgType& message);
  
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Debug_DevLoggingSystem_H_
