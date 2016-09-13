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

#include "taskExecutor.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace Anki
{
namespace Das
{

using DASLogFileConsumptionBlock = std::function<bool (const std::string& logFilePath, bool* stop)>;

class DasLogFileAppender
{
public:
  DasLogFileAppender(const std::string& logDirPath);
  ~DasLogFileAppender();
  std::string CurrentLogFilePath();
  void WriteDataToCurrentLogfile(const std::string logData);
  void RolloverAllLogFiles();
  void RolloverCurrentLogFile();
  void SetMaxLogLength(size_t maxLogLength);
  void Flush();
  void ConsumeLogFiles(DASLogFileConsumptionBlock ConsumptionBlock);

  static const size_t kDefaultMaxLogLength = 100 * 1024;
  static constexpr const char* kDasLogFileExtension = "das";
  static constexpr const char* kDasInProgressExtension = "das_inprogress";
  static const int kDasMaxLogFiles = 400;

private:
  bool CreateNewLogFile() const;
  void PrvFlush();
  void UpdateLogFilePath();
  bool CreateNewLogFileIfNecessary() const;
  void OpenHandleToCurrentLogFile();
  void UpdateLogFileHandle();
  std::string FullLogFilePath() const;
  std::string MakeLogFilePath(const std::string &logDir, uint32_t logNumber, const std::string &extension) const;
  uint32_t NextLogFileNumber();
  uint32_t NextAvailableLogFileNumber() const;
  std::ofstream& CurrentLogFileHandle();
  void PrvCurrentLogFilePath(std::string* logFilePath);
  void PrvRolloverCurrentLogFile();
  void RolloverLogFileAtPath(std::string path) const;
  std::vector<uint32_t> InProgressLogNumbers() const;

private:
  std::string _logDirPath;
  uint32_t _maxLogLength;
  uint64_t _bytesLoggedToCurrentFile;
  std::string _currentLogFileName;
  std::ofstream _currentLogFileHandle;
  uint32_t _currentLogFileNumber;
  TaskExecutor _loggingQueue;
};

#endif // __DasLogFileAppender_H__

} // namespace Das
} // namespace Anki

