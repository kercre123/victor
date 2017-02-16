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

DasLogFileAppender::DasLogFileAppender(const std::string& logDirPath)
: _logDirPath(logDirPath)
, _maxLogLength(DasLogFileAppender::kDefaultMaxLogLength)
, _bytesLoggedToCurrentFile(0)
, _currentLogFileName()
, _currentLogFileHandle()
, _currentLogFileNumber(0)
{
  _currentLogFileNumber = NextAvailableLogFileNumber();
  UpdateLogFilePath();
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

  pathStream << std::setfill('0') << std::setw(2) << logNumber << "." << extension;
  std::string path = pathStream.str();

  return path;
}

uint32_t DasLogFileAppender::NextLogFileNumber()
{
  _currentLogFileNumber++;
  if (_currentLogFileNumber >= DasLogFileAppender::kDasMaxLogFiles) {
    _currentLogFileNumber = 0;
  }
  return _currentLogFileNumber;
}

uint32_t DasLogFileAppender::NextAvailableLogFileNumber() const
{
  uint32_t fileNumber = 1;
  bool foundFileNumber = false;
  while (!foundFileNumber) {
    std::string candidateFinishedPath = MakeLogFilePath(_logDirPath, fileNumber, kDasLogFileExtension);
    if (!AnkiUtil::FileExistsAtPath(candidateFinishedPath)) {
      foundFileNumber = true;
    } else {
      fileNumber++;
    }

    if (fileNumber >= DasLogFileAppender::kDasMaxLogFiles) {
      // we'll blow away the first log..
      // FIXME: we should log the fact that we've rolled over, since it means we're losing data.
      fileNumber = 0;
      foundFileNumber = true;
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
  _currentLogFileNumber = NextLogFileNumber();
}

void DasLogFileAppender::RolloverLogFileAtPath(std::string path) const
{
  assert(stringEndsWith(path, kDasInProgressExtension));
  // replace the file extension with kDasInProgressExtension
  size_t pos = path.find_last_of('.');
  std::string completedLogPath = path.substr(0, pos + 1) + kDasLogFileExtension;
  (void) rename(path.c_str(), completedLogPath.c_str());
}

void DasLogFileAppender::RolloverAllLogFiles()
{
  _loggingQueue.Wake([this] {
      std::vector<uint32_t> toRollOverNumbers = InProgressLogNumbers();
      for (uint32_t i : toRollOverNumbers) {
        if (i != _currentLogFileNumber) {
          std::string path = MakeLogFilePath(_logDirPath, i, kDasInProgressExtension);
          RolloverLogFileAtPath(std::move(path));

        }
      }
    });
}

std::vector<uint32_t> DasLogFileAppender::InProgressLogNumbers() const
{
  std::vector<uint32_t> result;
  DIR* logDir = opendir(_logDirPath.c_str());

  if (logDir) {
    struct dirent* dirent;
    while ((dirent = readdir(logDir)) != nullptr) {
      std::string filename(dirent->d_name);
      if (DT_REG == dirent->d_type && stringEndsWith(filename, kDasInProgressExtension)) {
        size_t pos = filename.find_last_of('.');
        std::string logFileBaseName = filename.substr(0, pos);
        uint32_t logFileNumber = (uint32_t) std::stoul(logFileBaseName);
        result.push_back(logFileNumber);
      }
    }
    (void) closedir(logDir);
  }

  return result;
}

void DasLogFileAppender::SetMaxLogLength(size_t maxLogLength)
{
  _maxLogLength = (uint32_t) maxLogLength;
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
        while (!shouldStop && ((dirent = readdir(logDir)) != nullptr)) {
          std::string filename(dirent->d_name);
          if (DT_REG == dirent->d_type && stringEndsWith(filename, kDasLogFileExtension)) {
            std::string fullPath = _logDirPath + "/" + filename;
            bool shouldDelete = ConsumptionBlock(fullPath, &shouldStop);
            if (shouldDelete) {
              success = (0 == unlink(fullPath.c_str()));
              if (!success) {
                LOGD("Error removing file '%s': %s", fullPath.c_str(), strerror(errno));
              }
            }
          }
        }
        (void) closedir(logDir);
      }
    });
}

} // namespace Das
} // namespace Anki
