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
#ifndef __DasGameLogAppender_H__
#define __DasGameLogAppender_H__

#include "DAS.h"
#include "taskExecutor.h"
#include "DASPrivate.h"
#include <string>
#include <map>

namespace Anki
{
namespace Das
{

class DasGameLogAppender
{
public:
  DasGameLogAppender(const std::string& gameLogDir, const std::string& gameId);
  ~DasGameLogAppender();

  void append(DASLogLevel level, const char* eventName, const char* eventValue,
              ThreadId_t threadId, const char* file, const char* funct, int line,
              const std::map<std::string,std::string>* globals,
              const std::map<std::string,std::string>& data);
  std::string makeCsvRow(const char* eventName, const char* eventValue, ThreadId_t threadId,
                         const std::map<std::string,std::string>* globals,
                         const std::map<std::string,std::string>& data);
  std::string getValueForCsv(const char* key,
                             const std::map<std::string,std::string>* globals,
                             const std::map<std::string,std::string>& data);
  void escapeStringForCsv(std::string& value);
  void flush();
  static constexpr const char* kLogFileName = "dasMessages.csv";
  static constexpr const char* kCsvHeaderRow =
    "ts,tid,level,event,value,data,phys,phone,unit,app,user,apprun,game,group,session,messv";
private:
  std::string _gameLogDirPath;
  TaskExecutor _loggingQueue;
};

#endif // __DasGameLogAppender_H__

} // namespace Das
} // namespace Anki

