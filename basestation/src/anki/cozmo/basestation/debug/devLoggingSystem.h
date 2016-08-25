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
#include "util/logging/rollingFileLogger.h"

#include <memory>
#include <string>
#include <chrono>
#include <vector>

namespace Anki {
namespace Cozmo {
  
using DevLoggingClock = Util::RollingFileLogger::ClockType;

class DevLoggingSystem : Util::noncopyable {
public:
  static void CreateInstance(const std::string& loggingBaseDirectory, const std::string& appRunId);
  static void DestroyInstance();
  static DevLoggingSystem* GetInstance() { return sInstance; }
  static const DevLoggingClock::time_point& GetAppRunStartTime() { return kAppRunStartTime; }
  static uint32_t GetAppRunMilliseconds();
  
  ~DevLoggingSystem();
  
  template<typename MsgType>
  void LogMessage(const MsgType& message);
  
  const std::string& GetDevLoggingBaseDirectory() const { return _devLoggingBaseDirectory; }
  
  void PrepareForUpload(const std::string& namePrefix) const;
  void DeleteLog(const std::string& archiveFilename) const;
  std::vector<std::string> GetLogFilenamesForUpload() const;
  std::string GetAppRunId(const std::string& archiveFilename) const;
  
  static const std::string kPrintName;
  static const std::string kGameToEngineName;
  static const std::string kEngineToGameName;
  static const std::string kRobotToEngineName;
  static const std::string kEngineToRobogName;
  static const std::string kEngineToVizName;

private:
  static DevLoggingSystem* sInstance;
  static const DevLoggingClock::time_point kAppRunStartTime;
  static const std::string kWaitingForUploadDirName;
  static const std::string kArchiveExtensionString;
  static const std::string kAppRunExtension;

  std::unique_ptr<Util::RollingFileLogger>    _gameToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToGameLog;
  std::unique_ptr<Util::RollingFileLogger>    _robotToEngineLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToRobotLog;
  std::unique_ptr<Util::RollingFileLogger>    _engineToVizLog;

  std::string _allLogsBaseDirectory;
  std::string _devLoggingBaseDirectory;
  std::string _appRunId;
  
  DevLoggingSystem(const std::string& baseDirectory, const std::string& appRunId);
  
  void DeleteFiles(const std::string& baseDirectory, const std::string& extension) const;
  static void CopyFile(const std::string& sourceFile, const std::string& destination);
  void ArchiveDirectories(const std::string& baseDirectory, const std::vector<std::string>& excludeDirectories) const;
  static void ArchiveOneDirectory(const std::string& baseDirectory);
  std::string GetPathString(const std::string& base, const std::string& path) const;
  static std::string GetAppRunFilename(const std::string& archiveFilename);

  template<typename MsgType>
  std::string PrepareMessage(const MsgType& message) const;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__Basestation_Debug_DevLoggingSystem_H_
