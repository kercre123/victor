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

static std::atomic<uint32_t> sDasSequenceNumber = ATOMIC_VAR_INIT(0);

namespace Anki
{
namespace Das
{

DasAppender::DasAppender(const std::string& dasLogDir, const std::string& url,uint32_t flush_interval)
: _logFileAppender(nullptr)
, _url(url)
, _maxLogLength(DasLogFileAppender::kDefaultMaxLogLength)
, _isWaitingForFlush(false)
, _lastFlushFailed(false)
, _flushIntervalSeconds(flush_interval)
{
  LOGV("Storing DAS logs in '%s'", dasLogDir.c_str());
  LOGV("Setting DAS URL to '%s'", url.c_str());

  bool mkdirSuccess = (0 == mkdir(dasLogDir.c_str(), S_IRWXU)) || (EEXIST == errno);
  if (mkdirSuccess) {
    _logFileAppender = new Anki::Das::DasLogFileAppender(dasLogDir);
    _syncQueue.Wake([this]() {_logFileAppender->RolloverAllLogFiles(); Flush();});
  } else {
    LOGD("ERROR! Couldn't create DAS log directory('%s'): %s", dasLogDir.c_str(), strerror(errno));
    // TODO: Try to send this log message directly to the endpoint over the network
  }
}

DasAppender::~DasAppender()
{
  _syncQueue.WakeSync([this]() {delete _logFileAppender; _logFileAppender = nullptr;});
}

void DasAppender::SetMaxLogLength(size_t maxLogLength)
{
  _maxLogLength = maxLogLength;
  _logFileAppender->SetMaxLogLength(maxLogLength);
}

void DasAppender::append(DASLogLevel level, const char* eventName, const char* eventValue,
                         ThreadId_t threadId, const char* file, const char* funct, int line,
                         const std::map<std::string,std::string>* globals,
                         const std::map<std::string,std::string>& data) {

  uint32_t curSequence = sDasSequenceNumber++;

  auto* eventDictionaryPtr = new std::map<std::string, std::string>{*globals};
  eventDictionaryPtr->insert(data.begin(), data.end());
  eventDictionaryPtr->emplace(eventName, eventValue);

  _syncQueue.Wake([this, eventDictionaryPtr, curSequence]() {
    auto& eventDictionary = *eventDictionaryPtr;

    std::string logSequence = std::to_string(curSequence);
    eventDictionary[kSequenceGlobalKey] = logSequence;
    if( eventDictionary.end() == eventDictionary.find(kMessageVersionGlobalKey) ) {
      eventDictionary[kMessageVersionGlobalKey] = kMessageVersionGlobalValue;
    }

    std::string logData = AnkiUtil::StringMapToJson(eventDictionary) + ",";

    if (logData.size() <= _maxLogLength) {
      _logFileAppender->WriteDataToCurrentLogfile(logData);
      if (!DASNetworkingDisabled) {
        SetTimedFlush();
      }
    } else {
      LOGD("Error! Dropping message of length %zd", logData.size());
    }
    delete eventDictionaryPtr;
  });
}

void DasAppender::close() {
  LOGV("%s", "Closing the DAS Appender");
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

void DasAppender::ForceFlushWithCallback(const std::function<void()>& callback)
{
  _syncQueue.Wake([this, callback] {
    Flush();
    if (callback) {
      callback();
    }
  });
}

void DasAppender::Flush()
{
  _isWaitingForFlush = false;
  if (!_lastFlushFailed) {
    // Don't roll over the log if we're not actually able to hit the server.
    // This will help to maximize the number of logs we can store on disk.
    _logFileAppender->RolloverCurrentLogFile();
  }

  auto func = std::bind(&DasAppender::ConsumeALogFile, this, std::placeholders::_1, std::placeholders::_2);

  _logFileAppender->ConsumeLogFiles(func);
}

bool DasAppender::ConsumeALogFile(const std::string& logFilePath, bool *stop)
{
  if (DASNetworkingDisabled) {
    return false;
  }
  std::string logFileData = AnkiUtil::StringFromContentsOfFile(logFilePath);
  size_t logFileLength = logFileData.length();
  LOGV("Attempting to post a log file of size %zd", logFileLength);
  if (0 == logFileLength) {
    // empty log file, consider it consumed.
    return true;
  }

  logFileData.pop_back(); // strip off the last byte, it's a comma.

  std::string postBody = "[" + logFileData + "]";
  bool success = dasPostToServer(_url, postBody);
  // Stop consuming logs if we can't post
  if (success) {
    _lastFlushFailed = false;
  } else {
    *stop = true;
    _lastFlushFailed = true;
  }
  return success;
}


} // namespace Das
} // namespace Anki
