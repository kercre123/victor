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
  static void CreateInstance(const std::string& loggingBaseDirectory);
  static void DestroyInstance();
  static DevLoggingSystem* GetInstance() { return sInstance; }
  static const std::chrono::system_clock::time_point& GetAppRunStartTime() { return kAppRunStartTime; }
  
  virtual ~DevLoggingSystem();
  
  template<typename MsgType>
  void LogMessage(const MsgType& message);
  
  const std::string& GetDevLoggingBaseDirectory() { return _devLoggingBaseDirectory; }
  
private:
  static DevLoggingSystem* sInstance;
  static const std::chrono::system_clock::time_point kAppRunStartTime;
  
  std::unique_ptr<Util::RollingFileLogger>    _gameToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToGameLog;
  std::unique_ptr<Util::RollingFileLogger>    _robotToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToRobotLog;
  std::string _devLoggingBaseDirectory;
  
  DevLoggingSystem(const std::string& baseDirectory);
  
  void DeleteFiles(const std::string& baseDirectory, const std::string& extension);
  void ArchiveDirectories(const std::string& baseDirectory, const std::string& excludeDirectory);
  std::string GetPathString(const std::string& base, const std::string& path);
  
  template<typename MsgType>
  std::string PrepareMessage(const MsgType& message);
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Debug_DevLoggingSystem_H_
