/**
 * File: iFormattedLoggerProvider
 *
 * Author: Trevor Dasch
 * Created: 2/10/16
 *
 * Description: abstract class
 * that formats the log into a c-string
 * and calls its Log function.
 *
 * Copyright: Anki, inc. 2016
 *
 */
#ifndef __Util_Logging_IFormattedLoggerProvider_H_
#define __Util_Logging_IFormattedLoggerProvider_H_
#include "util/logging/iLoggerProvider.h"
#include <string>

namespace Anki {
  namespace Util {
    
    class IFormattedLoggerProvider : public ILoggerProvider {
      
    public:
      IFormattedLoggerProvider() : minLogLevel(LOG_LEVEL_DEBUG) {};
      
      inline void PrintLogE(const char* eventName,
                            const std::vector<std::pair<const char*, const char*>>& keyValues,
                            const char* eventValue) override {
        if (minLogLevel > LOG_LEVEL_ERROR) {return;}
        FormatAndLog(LOG_LEVEL_ERROR, eventName, keyValues, eventValue);
      }
      inline void PrintLogW(const char* eventName,
                            const std::vector<std::pair<const char*, const char*>>& keyValues,
                            const char* eventValue) override {
        if (minLogLevel > LOG_LEVEL_WARN) {return;}
        FormatAndLog(LOG_LEVEL_WARN, eventName, keyValues, eventValue);
      };
      inline void PrintEvent(const char* eventName,
                             const std::vector<std::pair<const char*, const char*>>& keyValues,
                             const char* eventValue) override {
        if (minLogLevel > LOG_LEVEL_EVENT) {return;}
        FormatAndLog(LOG_LEVEL_EVENT, eventName, keyValues, eventValue);
      };
      inline void PrintLogI(const char* eventName,
                            const std::vector<std::pair<const char*, const char*>>& keyValues,
                            const char* eventValue) override {
        if (minLogLevel > LOG_LEVEL_INFO) {return;}
        FormatAndLog(LOG_LEVEL_INFO, eventName, keyValues, eventValue);
      };
      inline void PrintLogD(const char* eventName,
                            const std::vector<std::pair<const char*, const char*>>& keyValues,
                            const char* eventValue) override {
        if (minLogLevel > LOG_LEVEL_DEBUG) {return;}
        FormatAndLog(LOG_LEVEL_DEBUG, eventName, keyValues, eventValue);
      }
      
      // debug == 1
      // info == 2
      // event == 3
      // warning == 4
      // error == 5
      inline void SetMinLogLevel(int logLevel) {minLogLevel = logLevel; };
      
      // This has to be public for MultiFormattedLoggerProvider to work.
      virtual void Log(ILoggerProvider::LogLevel logLevel, const std::string& message) = 0;

    private:
      void FormatAndLog(ILoggerProvider::LogLevel logLevel, const char* eventName,
                  const std::vector<std::pair<const char*, const char*>>& keyValues,
                  const char* eventValue);
      
      int minLogLevel;
    };
    
  } // end namespace Util
} // end namespace Anki


#endif //__Util_Logging_IFormattedLoggerProvider_H_
