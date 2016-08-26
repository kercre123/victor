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
#include "clad/vizInterface/messageViz.h"
#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/dispatchQueue/dispatchQueue.h"
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
  
const std::string DevLoggingSystem::kPrintName = "print";
const std::string DevLoggingSystem::kGameToEngineName = "gameToEngine";
const std::string DevLoggingSystem::kEngineToGameName = "engineToGame";
const std::string DevLoggingSystem::kRobotToEngineName = "robotToEngine";
const std::string DevLoggingSystem::kEngineToRobogName = "engineToRobot";
const std::string DevLoggingSystem::kEngineToVizName = "engineToViz";
const std::string DevLoggingSystem::kAppRunExtension = ".apprun";

void DevLoggingSystem::CreateInstance(const std::string& loggingBaseDirectory, const std::string& appRunId)
{
  Util::SafeDelete(sInstance);

  sInstance = new DevLoggingSystem(loggingBaseDirectory, appRunId);
}

void DevLoggingSystem::DestroyInstance()
{
  Util::SafeDelete(sInstance);
}

DevLoggingSystem::DevLoggingSystem(const std::string& baseDirectory, const std::string& appRunId)
: _queue(Util::Dispatch::Create("DevLogger"))
, _allLogsBaseDirectory(baseDirectory)
, _appRunId(appRunId)
{
  std::string appRunTimeString = Util::RollingFileLogger::GetDateTimeString(DevLoggingSystem::GetAppRunStartTime());

  // TODO:(lc) For the playtest we don't want to delete any log files, since they could be very valuable
  //DeleteFiles(_allLogsBaseDirectory, kArchiveExtensionString);
  ArchiveDirectories(_allLogsBaseDirectory, {appRunTimeString, kWaitingForUploadDirName} );
  
  _devLoggingBaseDirectory = GetPathString(_allLogsBaseDirectory, appRunTimeString);
  _gameToEngineLog.reset(new Util::RollingFileLogger(_queue, GetPathString(_devLoggingBaseDirectory, kGameToEngineName)));
  _engineToGameLog.reset(new Util::RollingFileLogger(_queue, GetPathString(_devLoggingBaseDirectory, kEngineToGameName)));
  _robotToEngineLog.reset(new Util::RollingFileLogger(_queue, GetPathString(_devLoggingBaseDirectory, kRobotToEngineName)));
  _engineToRobotLog.reset(new Util::RollingFileLogger(_queue, GetPathString(_devLoggingBaseDirectory, kEngineToRobogName)));
  _engineToVizLog.reset(new Util::RollingFileLogger(_queue, GetPathString(_devLoggingBaseDirectory, kEngineToVizName)));

  // write apprun file
  Util::FileUtils::WriteFile(GetPathString(_devLoggingBaseDirectory, appRunTimeString + kAppRunExtension), appRunId);
}
  
void DevLoggingSystem::DeleteFiles(const std::string& baseDirectory, const std::string& extension) const
{
  auto filesToDelete = Util::FileUtils::FilesInDirectory(baseDirectory, true, extension.c_str());
  for (auto& file : filesToDelete)
  {
    Util::FileUtils::DeleteFile(file);
  }
}

void DevLoggingSystem::CopyFile(const std::string& sourceFile, const std::string& destination)
{
  Util::FileUtils::WriteFile(destination, Util::FileUtils::ReadFile(sourceFile));
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
    {
      // copy apprun file where we need it
      auto appRunSources = Util::FileUtils::FilesInDirectory(directoryPath, true, kAppRunExtension.c_str());
      if (!appRunSources.empty())
      {
        CopyFile(appRunSources[0], directoryPath + kAppRunExtension);
      }
    }
    ArchiveOneDirectory(directoryPath);
    Util::FileUtils::RemoveDirectory(directoryPath);
  }
}

void DevLoggingSystem::ArchiveOneDirectory(const std::string& baseDirectory)
{
  auto filePaths = Util::FileUtils::FilesInDirectory(baseDirectory, true, {Util::RollingFileLogger::kDefaultFileExtension, kAppRunExtension.c_str()}, true);
  ArchiveUtil::CreateArchiveFromFiles(baseDirectory + kArchiveExtensionString, baseDirectory, filePaths);
}

void DevLoggingSystem::PrepareForUpload(const std::string& namePrefix) const
{
  // First create an archive for the current logs
  ArchiveOneDirectory(_devLoggingBaseDirectory);

  // Copy current apprun file up one dir
  {
    auto appRunSources = Util::FileUtils::FilesInDirectory(_devLoggingBaseDirectory, false, kAppRunExtension.c_str(), false);
    if (appRunSources.size() > 0)
    {
      CopyFile(GetPathString(_devLoggingBaseDirectory, appRunSources[0]), GetPathString(_allLogsBaseDirectory, appRunSources[0]));
    }
  }

  // Now move all existing archives to the upload directory
  auto filesToMove = Util::FileUtils::FilesInDirectory(_allLogsBaseDirectory, false, {kArchiveExtensionString.c_str(), kAppRunExtension.c_str()});
  
  // If we had nothing to move, we have nothing left to do
  if (filesToMove.empty())
  {
    return;
  }
  
  std::string waitingDir = GetPathString(_allLogsBaseDirectory, kWaitingForUploadDirName);
  Util::FileUtils::CreateDirectory(waitingDir);
  for (auto& filename : filesToMove)
  {
    std::string newFilename = GetPathString(waitingDir, namePrefix + filename);
    Util::FileUtils::DeleteFile(newFilename);
    std::rename(GetPathString(_allLogsBaseDirectory, filename).c_str(), newFilename.c_str());
  }
}

std::vector<std::string> DevLoggingSystem::GetLogFilenamesForUpload() const
{
  std::string waitingDir = GetPathString(_allLogsBaseDirectory, kWaitingForUploadDirName);
  return Util::FileUtils::FilesInDirectory(waitingDir, true, kArchiveExtensionString.c_str());
}

std::string DevLoggingSystem::GetAppRunId(const std::string& archiveFilename) const
{
  auto appRunFilename = GetAppRunFilename(archiveFilename);
  return Util::FileUtils::ReadFile(appRunFilename);
}

std::string DevLoggingSystem::GetAppRunFilename(const std::string& archiveFilename)
{
  return archiveFilename.substr(0, archiveFilename.find(kArchiveExtensionString)) + kAppRunExtension;
}

void DevLoggingSystem::DeleteLog(const std::string& archiveFilename) const
{
  Util::FileUtils::DeleteFile(GetAppRunFilename(archiveFilename));
  Util::FileUtils::DeleteFile(archiveFilename);
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

DevLoggingSystem::~DevLoggingSystem()
{
  Util::Dispatch::Stop(_queue);
  Util::Dispatch::Release(_queue);
}

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
  *((uint32_t*)data) = GetAppRunMilliseconds();
  data += sizeof(uint32_t);
  
  message.Pack(data, messageSize);
  
  return std::string(reinterpret_cast<char*>(messageVector.data()), totalSize);
}
  
uint32_t DevLoggingSystem::GetAppRunMilliseconds()
{
  return Util::numeric_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(DevLoggingClock::now() - kAppRunStartTime).count());
}

template<>
void DevLoggingSystem::LogMessage(const ExternalInterface::MessageEngineToGame& message)
{
  // Ignore ping messages - they're spammy, show up in profiles, and are uninteresting for message debugging / analysis
  if (ExternalInterface::MessageEngineToGameTag::Ping == message.GetTag())
  {
    return;
  }

  ANKI_CPU_PROFILE("LogMessage_EToG");
  _engineToGameLog->Write(PrepareMessage(message));
}
  
template<>
void DevLoggingSystem::LogMessage(const ExternalInterface::MessageGameToEngine& message)
{
  // Ignore ping messages - they're spammy, show up in profiles, and are uninteresting for message debugging / analysis
  if (ExternalInterface::MessageGameToEngineTag::Ping == message.GetTag())
  {
    return;
  }
  
  ANKI_CPU_PROFILE("LogMessage_GToE");
  _gameToEngineLog->Write(PrepareMessage(message));
}

template<>
void DevLoggingSystem::LogMessage(const RobotInterface::EngineToRobot& message)
{
  ANKI_CPU_PROFILE("LogMessage_EToR");
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
  
  ANKI_CPU_PROFILE("LogMessage_RToE");
  _robotToEngineLog->Write(PrepareMessage(message));
}
    
template<>
void DevLoggingSystem::LogMessage(const VizInterface::MessageViz& message)
{
  // Ignore image chunks for now since they are too large in size.
  if (VizInterface::MessageVizTag::ImageChunk == message.GetTag())
  {
    return;
  }
    
  ANKI_CPU_PROFILE("LogMessage_Viz");
  _engineToVizLog->Write(PrepareMessage(message));
}

} // end namespace Cozmo
} // end namespace Anki
