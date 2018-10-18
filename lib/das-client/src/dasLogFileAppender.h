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
#ifndef __DasLogFileAppender_H__
#define __DasLogFileAppender_H__

#include "DAS.h"
#include "DASPrivate.h"
#include "taskExecutor.h"
#include <iostream>
#include <fstream>
#include <functional>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

namespace Anki
{
namespace Das
{

using DASLogFileConsumptionBlock = std::function<bool (const std::string& logFilePath, bool* stop)>;

class DasLogFileAppender
{
public:
  DasLogFileAppender(const std::string& logDirPath,
                     size_t maxLogLength = kDefaultMaxLogLength,
                     size_t maxLogFiles = kDasDefaultMaxLogFiles,
                     const DASArchiveFunction& archiveCallback = DASArchiveFunction{},
                     const DASUnarchiveFunction& unarchiveCallback = DASUnarchiveFunction{},
                     const std::string& archiveFileExtension = "");
  
  ~DasLogFileAppender();
  std::string CurrentLogFilePath();
  void WriteDataToCurrentLogfile(const std::string logData);
  void RolloverAllLogFiles();
  void RolloverCurrentLogFile();
  void SetMaxLogLength(size_t maxLogLength);
  void Flush();
  void ConsumeLogFiles(DASLogFileConsumptionBlock ConsumptionBlock);

  static constexpr const char* kDasLogFileExtension = "das";
  static constexpr const char* kDasInProgressExtension = "das_inprogress";

private:
  bool CreateNewLogFile() const;
  void PrvFlush();
  void UpdateLogFilePath();
  bool CreateNewLogFileIfNecessary() const;
  void OpenHandleToCurrentLogFile();
  void UpdateLogFileHandle();
  std::string FullLogFilePath() const;
  std::string MakeLogFilePath(const std::string &logDir, uint32_t logNumber, const std::string &extension) const;
  uint32_t NextAvailableLogFileNumber(uint32_t startNumber) const;
  std::ofstream& CurrentLogFileHandle();
  std::string PrvCurrentLogFilePath();
  void PrvRolloverCurrentLogFile();
  void RolloverLogFileAtPath(std::string path) const;
  std::vector<std::string> InProgressLogFiles() const;

private:
  std::string _logDirPath;
  size_t _maxLogLength;
  size_t _maxLogFiles;
  uint64_t _bytesLoggedToCurrentFile;
  std::string _currentLogFileName;
  std::ofstream _currentLogFileHandle;
  uint32_t _currentLogFileNumber;
  TaskExecutor _loggingQueue;
  DASArchiveFunction _archiveCallback;
  DASUnarchiveFunction _unarchiveCallback;
  std::string _archiveFileExtension;
};

#endif // __DasLogFileAppender_H__

} // namespace Das
} // namespace Anki

