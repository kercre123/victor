/**
 * File: dasGameLogAppender
 *
 * Author: seichert
 * Created: 01/26/15
 *
 * Description: Write out DAS logs for a game to a file
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
#include "dasGameLogAppender.h"
#include "dasGlobals.h"
#include <sys/stat.h>
#include <iomanip>
#include <fstream>
#include <sstream>

namespace Anki
{
namespace Das
{

DasGameLogAppender::DasGameLogAppender(const std::string& gameLogDir, const std::string& gameId)
: _gameLogDirPath(gameLogDir)
{
  std::string logDir = gameLogDir + "/" + gameId;
  (void) mkdir(logDir.c_str(), S_IRWXU);
}
  
DasGameLogAppender::~DasGameLogAppender()
{
  flush();
}

void DasGameLogAppender::append(DASLogLevel level, const char* eventName, const char* eventValue,
                                ThreadId_t threadId, const char* file, const char* funct, int line,
                                const std::map<std::string,std::string>* globals,
                                const std::map<std::string,std::string>& data)
{
  if (nullptr == globals) {
    return;
  }
  auto it = globals->find(kGameIdGlobalKey);
  if (globals->end() == it) {
    return;
  }

  std::string gameId = it->second;
  std::string logMessage = makeCsvRow(eventName, eventValue, threadId, globals, data);

  auto l = [this, gameId = std::move(gameId), logMessage = std::move(logMessage)] {
    std::string logFilePath = _gameLogDirPath + "/" + gameId + "/" + kLogFileName;
    std::ofstream log(logFilePath, std::ios::app | std::ios::out);
    if (0 == log.tellp()) {
      log << kCsvHeaderRow << std::endl;
    }
    log << logMessage << std::endl;
  };
  _loggingQueue.Wake(l);
}

std::string DasGameLogAppender::makeCsvRow(const char* eventName, const char* eventValue, ThreadId_t threadId,
                                           const std::map<std::string,std::string>* globals,
                                           const std::map<std::string,std::string>& data)
{
  std::ostringstream oss;
  std::string event(eventName);
  std::string value(eventValue);

  escapeStringForCsv(event);
  escapeStringForCsv(value);

  // REMINDER: If we change the columns we are outputting or the order in which
  // they are output, we need to update the definition of kCsvHeaderRow in
  // dasGameLogAppender.h

  oss << getValueForCsv(kTimeStampGlobalKey, globals, data) << ",";
  oss << threadId << ",";
  oss << getValueForCsv(kMessageLevelGlobalKey, globals, data) << ",";
  oss << event << ",";
  oss << value << ",";
  oss << getValueForCsv(kDataGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kPhysicalIdGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kPhoneTypeGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kUnitIdGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kApplicationVersionGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kUserIdGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kApplicationRunGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kGameIdGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kGroupIdGlobalKey, globals, data) << ",";
  oss << getValueForCsv(kConnectedSessionIdGlobalKey, globals, data) << ",";
  oss << kMessageVersionGlobalValue;

  return oss.str();
}

std::string DasGameLogAppender::getValueForCsv(const char* key,
                                               const std::map<std::string,std::string>* globals,
                                               const std::map<std::string,std::string>& data)
{
  std::string value = "";

  auto it = data.find(key);
  if (data.end() != it) {
    value = it->second;
  } else {
    it = globals->find(key);
    if (globals->end() != it) {
      value = it->second;
    }
  }

  escapeStringForCsv(value);

  return value;
}

void DasGameLogAppender::escapeStringForCsv(std::string& value)
{
  bool needsToBeQuoted = false;

  std::size_t pos = value.find('\"');
  while (std::string::npos != pos)
  {
    needsToBeQuoted = true;
    value.insert(pos, "\"");
    pos += 2;
    if (pos <= value.length()) {
      pos = value.find('\"', pos);
    }
  }

  needsToBeQuoted = needsToBeQuoted
    || (std::string::npos != value.find(','))
    || (std::string::npos != value.find('\r'))
    || (std::string::npos != value.find('\n'));

  if (needsToBeQuoted) {
    value = "\"" + value + "\"";
  }
}

void DasGameLogAppender::flush()
{
  auto l = [] {};
  _loggingQueue.WakeSync(l);
}

} // namespace Das
} // namespace Anki

