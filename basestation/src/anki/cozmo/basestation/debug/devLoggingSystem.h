/**
* File: devLoggingSystem
*
* Author: Lee Crippen
* Created: 3/30/2016
*
* Description: System for collecting, archiving, and uploading logs useful for debugging during development.
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
#include <vector>

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
  
  const std::string& GetDevLoggingBaseDirectory() const { return _devLoggingBaseDirectory; }
  
  void PrepareForUpload(const std::string& namePrefix) const;
  
private:
  static DevLoggingSystem* sInstance;
  static const std::chrono::system_clock::time_point kAppRunStartTime;
  static const std::string kWaitingForUploadDirName;
  static const std::string kArchiveExtensionString;
  
  std::unique_ptr<Util::RollingFileLogger>    _gameToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToGameLog;
  std::unique_ptr<Util::RollingFileLogger>    _robotToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToRobotLog;
  
  std::string _allLogsBaseDirectory;
  std::string _devLoggingBaseDirectory;
  
  DevLoggingSystem(const std::string& baseDirectory);
  
  void DeleteFiles(const std::string& baseDirectory, const std::string& extension) const;
  void ArchiveDirectories(const std::string& baseDirectory, const std::vector<std::string>& excludeDirectories) const;
  std::string GetPathString(const std::string& base, const std::string& path) const;
  
  template<typename MsgType>
  std::string PrepareMessage(const MsgType& message) const;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Debug_DevLoggingSystem_H_
