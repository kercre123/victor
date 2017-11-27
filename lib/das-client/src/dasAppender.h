/**
 * File: dasAppender
 *
 * Author: seichert
 * Created: 07/10/14
 *
 * Description: Appender to log on remote DAS server
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#ifndef __DasAppender_H__
#define __DasAppender_H__

#include "DAS.h"
#include "DASPrivate.h"
#include "taskExecutor.h"
#include <functional>
#include <string>
#include <map>

namespace Anki
{
namespace Das
{
class DasLogFileAppender;

class DasAppender {
public:
  static const uint32_t kDefaultFlushIntervalSeconds = 5;
  
  DasAppender(const std::string& dasLogDir,
              const std::string& url,
              uint32_t flush_interval = kDefaultFlushIntervalSeconds,
              size_t maxLogLength = kDefaultMaxLogLength,
              size_t maxLogFiles = kDasDefaultMaxLogFiles,
              const DASArchiveFunction& archiveCallback = DASArchiveFunction{},
              const DASUnarchiveFunction& unarchiveCallback = DASUnarchiveFunction{},
              const std::string& archiveFileExtension = "");
  ~DasAppender();
  void SetMaxLogLength(size_t maxLogLength);

  void append(DASLogLevel level, const char* eventName, const char* eventValue,
              ThreadId_t threadId, const char* file, const char* funct, int line,
              const std::map<std::string,std::string>* globals,
              const std::map<std::string,std::string>& data);
  void append(DASLogLevel level, const char* eventName, const char* eventValue,
              const std::map<std::string,std::string>& data) {
    append(level, eventName, eventValue, 0, nullptr, nullptr, 0, nullptr, data);
  }

  void ForceFlush();
  void ForceFlushWithCallback(const DASFlushCallback& completionBlock);

  void SetIsUploadingPaused(const bool isPaused);

private:
  void SetTimedFlush();
  bool ConsumeALogFile(const std::string &logFilePath, bool *stop);
  void Flush();
  DasLogFileAppender* _logFileAppender;
  std::string _url;
  size_t _maxLogLength;
  bool _isWaitingForFlush;
  bool _lastFlushFailed;
  std::string _lastFlushResponse;
  bool _isUploadingPaused;
  uint32_t _flushIntervalSeconds;
  TaskExecutor _syncQueue;
};

} // namespace Das
} // namespace Anki

#endif // __DasAppender_H__
