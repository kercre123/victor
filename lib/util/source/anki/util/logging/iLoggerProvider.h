/**
* File: iLoggerProvider
*
* Author: damjan
* Created: 4/21/2015
*
* Description: interface for anki log
*
* Copyright: Anki, Inc. 2014
*
**/

#ifndef __Util_Logging_ILoggerProvider_H__
#define __Util_Logging_ILoggerProvider_H__

#include <vector>

namespace Anki {
namespace Util {

class ILoggerProvider {
public:
  
  typedef enum {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_EVENT,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
  } LogLevel;
  
  virtual ~ILoggerProvider(){};
  virtual void PrintEvent(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;
  virtual void PrintLogE(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;
  virtual void PrintLogW(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;
  virtual void PrintLogI(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;
  virtual void PrintLogD(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;
  
  const char* GetLogLevelString(LogLevel level) {return _logLevelStrings[level];}
  
private:
  const char* _logLevelStrings[LOG_LEVEL_ERROR+1] = {"Debug", "Info", "Event", "Warn", "Error"};
};



} // namespace Util
} // namespace Anki

#endif // __Util_Logging_ILoggerProvider_H__

