/**
 * File: dasLogFileAppender
 *
 * Author: seichert
 * Created: 07/10/14
 *
 * Description: Write out DAS logs to a file
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#include "dasLogFileAppender.h"
#include "dasLogMacros.h"
#include "DAS.h"
#include "fileUtils.h"

#include <cassert>
#include <cinttypes>
#include <functional>
#include <future>
#include <iomanip>
#include <mutex>
#include <sstream>

#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

namespace Anki
{
namespace Das
{

static bool stringEndsWith(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

DasLogFileAppender::DasLogFileAppender(const std::string& logDirPath,
                                       size_t maxLogLength,
                                       size_t maxLogFiles,
                                       const DASArchiveFunction& archiveCallback,
                                       const DASUnarchiveFunction& unarchiveCallback,
                                       const std::string& archiveFileExtension)
: _logDirPath(logDirPath)
, _maxLogLength(maxLogLength)
, _maxLogFiles(maxLogFiles)
, _bytesLoggedToCurrentFile(0)
, _currentLogFileName()
, _currentLogFileHandle()
, _currentLogFileNumber(0)
, _archiveCallback(archiveCallback)
, _unarchiveCallback(unarchiveCallback)
, _archiveFileExtension(archiveFileExtension)
{
  if (_maxLogLength > kDasMaxAllowableLogLength)
  {
    _maxLogLength = kDasMaxAllowableLogLength;
  }
  _currentLogFileNumber = NextAvailableLogFileNumber(_currentLogFileNumber + 1);
  UpdateLogFileHandle();
}

DasLogFileAppender::~DasLogFileAppender()
{
  _loggingQueue.StopExecution();
  _currentLogFileHandle.close();
}

bool DasLogFileAppender::CreateNewLogFile() const
{
  int fd;

  fd = open(FullLogFilePath().c_str(), O_CREAT | O_TRUNC | O_WRONLY, S_IRWXU);

  if (fd > 0) {
    (void) close(fd);
    return true;
  }

  return false;
}

void DasLogFileAppender::UpdateLogFilePath()
{
  _currentLogFileName = MakeLogFilePath("", _currentLogFileNumber, kDasInProgressExtension);
}

bool DasLogFileAppender::CreateNewLogFileIfNecessary() const
{
  bool createdNewFile = false;

  if (!AnkiUtil::FileExistsAtPath(FullLogFilePath())) {
    if (CreateNewLogFile()) {
      createdNewFile = true;
    } else {
      LOGD("Error creating file at path! %s", FullLogFilePath().c_str());
    }
  }
  return createdNewFile;
}

void DasLogFileAppender::OpenHandleToCurrentLogFile()
{
  _currentLogFileHandle.open(FullLogFilePath(), std::ofstream::out | std::ofstream::app);
  if (!_currentLogFileHandle) {
    LOGD("Error getting handle for file %s: %s !!", FullLogFilePath().c_str(), strerror(errno));
  }
}

void DasLogFileAppender::UpdateLogFileHandle()
{
  bool createdNewFile = false;
  // Make sure the log file actually exists
  UpdateLogFilePath();
  createdNewFile = CreateNewLogFileIfNecessary();
  OpenHandleToCurrentLogFile();
  if (!createdNewFile) {
    _currentLogFileHandle.seekp(0, std::ios_base::end);
    _bytesLoggedToCurrentFile = _currentLogFileHandle.tellp();
  }
}

std::string DasLogFileAppender::FullLogFilePath() const
{
  std::ostringstream fullPathStream;
  fullPathStream << _logDirPath << "/" << _currentLogFileName;
  std::string fullPath = fullPathStream.str();
  return fullPath;
}

std::string DasLogFileAppender::MakeLogFilePath(const std::string &logDir,
                                                uint32_t logNumber,
                                                const std::string &extension) const
{
  std::ostringstream pathStream;

  if (!logDir.empty()) {
    pathStream << logDir << '/';
  }

  pathStream << std::setfill('0') << std::setw(4) << logNumber << "." << extension;
  std::string path = pathStream.str();

  return path;
}

uint32_t DasLogFileAppender::NextAvailableLogFileNumber(uint32_t startNumber) const
{
  // Make sure we aren't looking over our limit
  startNumber = startNumber % _maxLogFiles;
  
  uint32_t fileNumber = startNumber;
  bool foundFileNumber = false;
  while (!foundFileNumber) {
    
    // Lets be optimistic to simplify some logic
    foundFileNumber = true;
    
    // Check for a file that exists without any archive extension
    std::string candidatePath = MakeLogFilePath(_logDirPath, fileNumber, kDasLogFileExtension);
    if (AnkiUtil::FileExistsAtPath(candidatePath)) {
      foundFileNumber = false;
    }
    
    // Check for a file that exists with an archive extension
    if (foundFileNumber && !_archiveFileExtension.empty()) {
      const auto fullArchiveExtension = std::string(kDasLogFileExtension + _archiveFileExtension);
      candidatePath = MakeLogFilePath(_logDirPath, fileNumber, fullArchiveExtension);
      if (AnkiUtil::FileExistsAtPath(candidatePath))
      {
        foundFileNumber = false;
      }
    }
    
    // Check for a file that exists with an in-progress extension
    if (foundFileNumber) {
      candidatePath = MakeLogFilePath(_logDirPath, fileNumber, kDasInProgressExtension);
      if (AnkiUtil::FileExistsAtPath(candidatePath))
      {
        foundFileNumber = false;
      }
    }
    
    // Otherwise keep looking
    if (!foundFileNumber) {
      fileNumber++;
      
      if (fileNumber >= _maxLogFiles) {
        fileNumber = 0;
      }
      
      if (fileNumber == startNumber)
      {
        // If we've wrapped around to the number we started with, give up trying
        // FIXME: we should log the fact that we've rolled over, since it means we're losing data.
        foundFileNumber = true;
      }
    }

  }
  return fileNumber;
}

std::ofstream& DasLogFileAppender::CurrentLogFileHandle()
{
  if (!_currentLogFileHandle || !_currentLogFileHandle.is_open()) {
    UpdateLogFileHandle();
  }
  return _currentLogFileHandle;
}

std::string DasLogFileAppender::CurrentLogFilePath()
{
  std::string logFilePath;
  _loggingQueue.WakeSync([this, &logFilePath] {
    logFilePath = PrvCurrentLogFilePath();
  });
  return logFilePath;
}

std::string DasLogFileAppender::PrvCurrentLogFilePath()
{
  if (_currentLogFileName.empty()) {
    UpdateLogFilePath();
  }
  std::string logFilePath = FullLogFilePath();
  return logFilePath;
}

void DasLogFileAppender::WriteDataToCurrentLogfile(std::string logData)
{
  _loggingQueue.Wake([this, logData = std::move(logData)] {
      if (logData.length() + _bytesLoggedToCurrentFile > _maxLogLength) {
        PrvRolloverCurrentLogFile();
      }

      CurrentLogFileHandle() << logData;
      CurrentLogFileHandle().flush();
      _bytesLoggedToCurrentFile += logData.length();
    });
}

void DasLogFileAppender::PrvFlush()
{
  CurrentLogFileHandle().flush();
}

void DasLogFileAppender::Flush()
{
  _loggingQueue.WakeSync([this] {PrvFlush();});
}

void DasLogFileAppender::RolloverCurrentLogFile()
{
  _loggingQueue.Wake([this] {
      if (_bytesLoggedToCurrentFile > 0) {
        PrvFlush();
        PrvRolloverCurrentLogFile();
      }
    });
}

void DasLogFileAppender::PrvRolloverCurrentLogFile()
{
  _currentLogFileHandle.close();
  std::string inProgressLogPath = PrvCurrentLogFilePath();

  RolloverLogFileAtPath(inProgressLogPath);

  _currentLogFileName.clear();
  _bytesLoggedToCurrentFile = 0;
  _currentLogFileNumber = NextAvailableLogFileNumber(_currentLogFileNumber + 1);
}

void DasLogFileAppender::RolloverLogFileAtPath(std::string path) const
{
  assert(stringEndsWith(path, kDasInProgressExtension));
  // replace the file extension with kDasInProgressExtension
  size_t pos = path.find_last_of('.');
  std::string completedLogPath = path.substr(0, pos + 1) + kDasLogFileExtension;
  (void) rename(path.c_str(), completedLogPath.c_str());
  
  // If we have a callback for archiving the log, call it here:
  if (_archiveCallback)
  {
    (void) _archiveCallback(completedLogPath);
  }
}

void DasLogFileAppender::RolloverAllLogFiles()
{
  _loggingQueue.Wake([this] {
      std::vector<std::string> toRollOverFiles = InProgressLogFiles();
      for (const auto& filename : toRollOverFiles) {
        size_t pos = filename.find_last_of('.');
        std::string logFileBaseName = filename.substr(0, pos);
        uint32_t logFileNumber = (uint32_t) std::stoul(logFileBaseName);
        if (logFileNumber != _currentLogFileNumber) {
          std::string path = _logDirPath + "/" + filename;
          RolloverLogFileAtPath(std::move(path));
        }
      }
    });
}

std::vector<std::string> DasLogFileAppender::InProgressLogFiles() const
{
  std::vector<std::string> result;
  DIR* logDir = opendir(_logDirPath.c_str());

  if (logDir) {
    struct dirent* dirent;
    while ((dirent = readdir(logDir)) != nullptr) {
      std::string filename(dirent->d_name);
      if (DT_REG == dirent->d_type && stringEndsWith(filename, kDasInProgressExtension)) {
        result.push_back(filename);
      }
    }
    (void) closedir(logDir);
  }

  return result;
}

void DasLogFileAppender::SetMaxLogLength(size_t maxLogLength)
{
  if (maxLogLength > kDasMaxAllowableLogLength)
  {
    _maxLogLength = kDasMaxAllowableLogLength;
  }
  else
  {
    _maxLogLength = (uint32_t) maxLogLength;
  }
}


// synchronous
// Don't want to block the logging queue..
void DasLogFileAppender::ConsumeLogFiles(DASLogFileConsumptionBlock ConsumptionBlock)
{
  _loggingQueue.WakeSync([this, ConsumptionBlock = std::move(ConsumptionBlock)] {
      DIR* logDir = opendir(_logDirPath.c_str());

      if (logDir) {
        struct dirent* dirent;
        bool shouldStop = false;
        bool success = true;
        std::vector<std::string> fileList;
        while ((dirent = readdir(logDir)) != nullptr) {
          if (DT_REG == dirent->d_type) {
            fileList.push_back(dirent->d_name);
          }
        }
        std::sort(fileList.begin(), fileList.end());
        auto fileIter = fileList.begin();
        while (!shouldStop && fileIter != fileList.end())
        {
          std::string filename = *fileIter;
          ++fileIter;
          // If we have a callback set for unarchiving and this file matches, unarchive it
          if (_unarchiveCallback && stringEndsWith(filename, _archiveFileExtension))
          {
            filename = _unarchiveCallback(_logDirPath + "/" + filename);
          }
          
          if (stringEndsWith(filename, kDasLogFileExtension)) {
            std::string fullPath = _logDirPath + "/" + filename;
            bool shouldDelete = ConsumptionBlock(fullPath, &shouldStop);
            if (shouldDelete) {
              success = (0 == unlink(fullPath.c_str()));
              if (!success) {
                LOGD("Error removing file '%s': %s", fullPath.c_str(), strerror(errno));
              }
            }
            // If we didn't successfully consume the file, archive it again
            else if (_archiveCallback) {
              (void) _archiveCallback(fullPath);
            }
          }
        }
        (void) closedir(logDir);
      }
    });
}

} // namespace Das
} // namespace Anki
