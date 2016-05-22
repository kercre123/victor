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
#include "anki/cozmo/basestation/debug/devLoggingSystem.h"
#include "anki/cozmo/basestation/util/file/archiveUtil.h"
#include "clad/externalInterface/messageEngineToGame.h"
#include "clad/externalInterface/messageGameToEngine.h"
#include "clad/robotInterface/messageEngineToRobot.h"
#include "clad/robotInterface/messageRobotToEngine.h"
#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/templateHelpers.h"
#include "util/logging/rollingFileLogger.h"

#include <sstream>
#include <cstdio>

namespace Anki {
namespace Cozmo {
  
DevLoggingSystem* DevLoggingSystem::sInstance                                   = nullptr;
const DevLoggingClock::time_point DevLoggingSystem::kAppRunStartTime            = DevLoggingClock::now();
const std::string DevLoggingSystem::kWaitingForUploadDirName                    = "waiting_for_upload";
const std::string DevLoggingSystem::kArchiveExtensionString                     = ".tar.gz";

void DevLoggingSystem::CreateInstance(const std::string& loggingBaseDirectory)
{
  Util::SafeDelete(sInstance);

  sInstance = new DevLoggingSystem(loggingBaseDirectory);
}

void DevLoggingSystem::DestroyInstance()
{
  Util::SafeDelete(sInstance);
}

DevLoggingSystem::DevLoggingSystem(const std::string& baseDirectory)
{
  DeleteFiles(baseDirectory, kArchiveExtensionString);
  std::string appRunTimeString = Util::RollingFileLogger::GetDateTimeString(DevLoggingSystem::GetAppRunStartTime());
  ArchiveDirectories(baseDirectory, {appRunTimeString, kWaitingForUploadDirName} );
  
  _allLogsBaseDirectory = baseDirectory;
  _devLoggingBaseDirectory = GetPathString(baseDirectory, appRunTimeString);
  
  _gameToEngineLog.reset(new Util::RollingFileLogger(GetPathString(_devLoggingBaseDirectory, "gameToEngine")));
  _engineToGameLog.reset(new Util::RollingFileLogger(GetPathString(_devLoggingBaseDirectory, "engineToGame")));
  _robotToEngineLog.reset(new Util::RollingFileLogger(GetPathString(_devLoggingBaseDirectory, "robotToEngine")));
  _engineToRobotLog.reset(new Util::RollingFileLogger(GetPathString(_devLoggingBaseDirectory, "engineToRobot")));
}
  
void DevLoggingSystem::DeleteFiles(const std::string& baseDirectory, const std::string& extension) const
{
  auto filesToDelete = Util::FileUtils::FilesInDirectory(baseDirectory, true, extension.c_str());
  for (auto& file : filesToDelete)
  {
    Util::FileUtils::DeleteFile(file);
  }
}
  
void DevLoggingSystem::ArchiveDirectories(const std::string& baseDirectory, const std::vector<std::string>& excludeDirectories) const
{
  std::vector<std::string> directoryList;
  Util::FileUtils::ListAllDirectories(baseDirectory, directoryList);
  
  for (auto& excludeDirectory : excludeDirectories)
  {
    for (auto iter = directoryList.begin(); iter != directoryList.end(); iter++)
    {
      if (*iter == excludeDirectory)
      {
        directoryList.erase(iter);
        break;
      }
    }
  }
  
  for (auto& directory : directoryList)
  {
    auto directoryPath = GetPathString(baseDirectory, directory);
    auto filePaths = Util::FileUtils::FilesInDirectory(directoryPath, true, Util::RollingFileLogger::kDefaultFileExtension, true);
    ArchiveUtil::CreateArchiveFromFiles(directoryPath + kArchiveExtensionString, directoryPath, filePaths);
    Util::FileUtils::RemoveDirectory(directoryPath);
  }
}
  
void DevLoggingSystem::PrepareForUpload(const std::string& namePrefix) const
{
  // TODO:(lc) Use the name prefix arg to either directly change the file names being saved or simply upload them with the name
  
  // First create an archive for the current logs
  auto filePaths = Util::FileUtils::FilesInDirectory(_devLoggingBaseDirectory, true, Util::RollingFileLogger::kDefaultFileExtension, true);
  ArchiveUtil::CreateArchiveFromFiles(_devLoggingBaseDirectory + kArchiveExtensionString, _devLoggingBaseDirectory, filePaths);
  
  // Now move all existing archives to the upload directory
  auto filesToMove = Util::FileUtils::FilesInDirectory(_allLogsBaseDirectory, false, kArchiveExtensionString.c_str());
  
  // If we had nothing to move, we have nothing left to do
  if (filesToMove.empty())
  {
    return;
  }
  
  std::string waitingDir = GetPathString(_allLogsBaseDirectory, kWaitingForUploadDirName);
  Util::FileUtils::CreateDirectory(waitingDir);
  for (auto& filename : filesToMove)
  {
    std::string newFilename = GetPathString(waitingDir, filename);
    Util::FileUtils::DeleteFile(newFilename);
    std::rename(GetPathString(_allLogsBaseDirectory, filename).c_str(), newFilename.c_str());
  }
  
  // TODO:(lc) Post all to amazon
}

std::string DevLoggingSystem::GetPathString(const std::string& base, const std::string& path) const
{
  std::ostringstream pathStream;
  if (!base.empty())
  {
    pathStream << base << '/';
  }
  
  pathStream << path;
  return pathStream.str();
}

DevLoggingSystem::~DevLoggingSystem() = default;

template<typename MsgType>
std::string DevLoggingSystem::PrepareMessage(const MsgType& message) const
{
  // We want to repackage the clad messages with some extra information at the start
  // We'll add 4 bytes to hold the size and another 4 for the timestamp
  static const std::size_t extraSize = sizeof(uint32_t) * 2;
  const std::size_t messageSize = message.Size();
  const std::size_t totalSize = messageSize + extraSize;
  std::vector<uint8_t> messageVector(totalSize);
  
  uint8_t* data = messageVector.data();
  
  // First write in the size
  *((uint32_t*)data) = static_cast<uint32_t>(totalSize);
  data += sizeof(uint32_t);
  
  // Then write in the timestamp as a millisecond count since the app started running
  auto timeNow = DevLoggingClock::now();
  auto msElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - kAppRunStartTime);
  *((uint32_t*)data) = Util::numeric_cast<uint32_t>(msElapsed.count());
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
  // CLAD messages from the robot with image payloads are large, and in chunks, so throw them out for now
  if (RobotInterface::RobotToEngineTag::image == message.GetTag())
  {
    return;
  }
  
  _robotToEngineLog->Write(PrepareMessage(message));
}
  
#if REMOTE_CONSOLE_ENABLED
void UploadDevLogs( ConsoleFunctionContextRef context )
{
  auto namePrefix = ConsoleArg_GetOptional_String( context, "namePrefix", "" );
  
  auto devLogging = DevLoggingSystem::GetInstance();
  if (nullptr != devLogging)
  {
    devLogging->PrepareForUpload(namePrefix);
  }
}
CONSOLE_FUNC( UploadDevLogs, "Dev", optional const char* namePrefix );
#endif


} // end namespace Cozmo
} // end namespace Anki
