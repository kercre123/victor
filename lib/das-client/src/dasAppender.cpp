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

#include "dasAppender.h"
#include "dasGlobals.h"
#include "dasLogFileAppender.h"
#include "dasLogMacros.h"
#include "DAS.h"
#include "dasPostToServer.h"
#include "DASPrivate.h"
#include "stringUtils.h"
#include <sys/stat.h>
#include <atomic>
#include <cstring>

static std::atomic<uint32_t> sDasSequenceNumber = ATOMIC_VAR_INIT(0);

namespace Anki
{
namespace Das
{

DasAppender::DasAppender(const std::string& dasLogDir,
                         const std::string& url,
                         uint32_t flush_interval,
                         size_t maxLogLength,
                         size_t maxLogFiles,
                         const DASArchiveFunction& archiveFunction,
                         const DASUnarchiveFunction& unarchiveFunction,
                         const std::string& archiveFileExtension)
: _logFileAppender(nullptr)
, _url(url)
, _maxLogLength(maxLogLength)
, _isWaitingForFlush(false)
, _lastFlushFailed(false)
, _lastFlushResponse("")
, _isUploadingPaused(true)
, _flushIntervalSeconds(flush_interval)
{
  LOGV("Storing DAS logs in '%s'", dasLogDir.c_str());
  LOGV("Setting DAS URL to '%s'", url.c_str());

  bool mkdirSuccess = (0 == mkdir(dasLogDir.c_str(), S_IRWXU)) || (EEXIST == errno);
  if (mkdirSuccess) {
    _logFileAppender = new Anki::Das::DasLogFileAppender(dasLogDir, maxLogLength, maxLogFiles,
                                                         archiveFunction, unarchiveFunction, archiveFileExtension);
    _syncQueue.Wake([this]() {_logFileAppender->RolloverAllLogFiles(); Flush();});
  } else {
    LOGD("ERROR! Couldn't create DAS log directory('%s'): %s", dasLogDir.c_str(), strerror(errno));
    // TODO: Try to send this log message directly to the endpoint over the network
  }
}

DasAppender::~DasAppender()
{
  _syncQueue.WakeSync([this]() {
    delete _logFileAppender;
    _logFileAppender = nullptr;
  });
}

void DasAppender::SetMaxLogLength(size_t maxLogLength)
{
  _maxLogLength = maxLogLength;
  if (_logFileAppender != nullptr) {
    _logFileAppender->SetMaxLogLength(maxLogLength);
  }
}

void DasAppender::append(DASLogLevel level, const char* eventName, const char* eventValue,
                         ThreadId_t threadId, const char* file, const char* funct, int line,
                         const std::map<std::string,std::string>* globals,
                         const std::map<std::string,std::string>& data) {

  if (DASNetworkingDisabled) { // reject logs if we've disabled DAS networking.
    return;
  }
  uint32_t curSequence = sDasSequenceNumber++;

  // In case of any overlap, values passed in `data` take precedence
  // over values in `globals`, so apply those first and only `insert`
  // global values if they don't already exist
  std::map<std::string, std::string> eventDictionary = data;
  if (globals) {
    eventDictionary.insert(globals->begin(), globals->end());
  }
  eventDictionary[eventName] = eventValue;

  _syncQueue.Wake([this, eventDictionary = std::move(eventDictionary), curSequence]() mutable {

      if (eventDictionary.end() == eventDictionary.find(kSequenceGlobalKey)) {
        std::string logSequence = std::to_string(curSequence);
        eventDictionary[kSequenceGlobalKey] = logSequence;
      }
      if( eventDictionary.end() == eventDictionary.find(kMessageVersionGlobalKey) ) {
        eventDictionary[kMessageVersionGlobalKey] = kMessageVersionGlobalValue;
      }

      std::string logData = AnkiUtil::StringMapToJson(eventDictionary) + ",";

      if (logData.size() <= _maxLogLength) {
        if (_logFileAppender != nullptr) {
          _logFileAppender->WriteDataToCurrentLogfile(logData);
        }
        if (!DASNetworkingDisabled) {
          SetTimedFlush();
        }
      } else {
        LOGD("Error! Dropping message of length %zd", logData.size());
      }
    });
}

void DasAppender::SetTimedFlush()
{
  // if flush interval is 0, ForceFlush has to be called.
  if (!_isWaitingForFlush && _flushIntervalSeconds != 0) {
    _isWaitingForFlush = true;
    auto when = std::chrono::system_clock::now() + std::chrono::seconds(_flushIntervalSeconds);
     _syncQueue.WakeAfter([this]() {Flush();}, when);

  }
}
  
void DasAppender::ForceFlush()
{
  ForceFlushWithCallback({});
}

void DasAppender::ForceFlushWithCallback(const DASFlushCallback& callback)
{
  _syncQueue.Wake([this, callback] {
    Flush();
    if (callback) {
      callback(!_lastFlushFailed, _lastFlushResponse);
    }
  });
}

void DasAppender::Flush()
{
  _isWaitingForFlush = false;
  if (!_lastFlushFailed) {
    // Don't roll over the log if we're not actually able to hit the server.
    // This will help to maximize the number of logs we can store on disk.
    if (_logFileAppender != nullptr) {
      _logFileAppender->RolloverCurrentLogFile();
    }
  }

  if (_logFileAppender != nullptr) {
    if (!_isUploadingPaused) {
      auto func = std::bind(&DasAppender::ConsumeALogFile, this, std::placeholders::_1, std::placeholders::_2);
      _logFileAppender->ConsumeLogFiles(func);
    }
  }
}

bool DasAppender::ConsumeALogFile(const std::string& logFilePath, bool *stop)
{
  if (DASNetworkingDisabled || _isUploadingPaused) {
    *stop = true;
    return false;
  }
  
  std::string logFileData = AnkiUtil::StringFromContentsOfFile(logFilePath);
  size_t logFileLength = logFileData.length();
  LOGV("Attempting to post a log file of size %zd", logFileLength);
  if (0 == logFileLength) {
    // empty log file, consider it consumed.
    return true;
  }
  
  if (logFileLength > kDasMaxAllowableLogLength)
  {
    // log file is too big. Pretend it was consumed so it will be removed.
    DASError("dasappender.consumealogfile.filetoobig",
             "File %s too big (%zu bytes with max %zu). Pretending it was consumed so it will be deleted.",
             logFilePath.c_str(), logFileLength, kDasMaxAllowableLogLength);
    return true;
  }

  logFileData.pop_back(); // strip off the last byte, it's a comma.

  std::string postBody = "[" + logFileData + "]";
  std::string postResponse = "";
  bool success = dasPostToServer(_url, postBody, postResponse);
  _lastFlushResponse = postResponse;
  // Stop consuming logs if we can't post
  if (success) {
    _lastFlushFailed = false;
    DASEvent("dasappender.postdasdata.success",
             "%s %s",
             logFilePath.c_str(), postResponse.c_str());
  } else {
    *stop = true;
    _lastFlushFailed = true;
    if (postResponse.empty())
    {
      DASEvent("dasappender.postdasdata.failurenoresponse",
               "%s",
               logFilePath.c_str());
    }
    else
    {
      DASEvent("dasappender.postdasdata.failurewithresponse",
               "%s %s",
               logFilePath.c_str(), postResponse.c_str());
    }
  }
  return success;
}

void DasAppender::SetIsUploadingPaused(const bool isPaused)
{
  const bool prevPaused = _isUploadingPaused;
  _isUploadingPaused = isPaused;
  // If resuming, attempt uploading again immediately
  if (!_isUploadingPaused && prevPaused) {
    ForceFlush();
  }
}


} // namespace Das
} // namespace Anki
