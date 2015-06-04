/**
* File: unityLoggerProvider
*
* Author: Greg Nagel
* Created: 6/4/2015
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/
#ifndef __CSharpBinding__unity_logger_provider_h_
#define __CSharpBinding__unity_logger_provider_h_

#include "csharp-binding.h"
#include "util/logging/iLoggerProvider.h"
#include <mutex>

namespace Anki {
namespace Cozmo {

class UnityLoggerProvider : public Anki::Util::ILoggerProvider {

public:
  UnityLoggerProvider() : minLogLevel(2) {};

  inline void PrintEvent(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    if (minLogLevel > 3) {return;}
    Write(3, "Event", eventName, keyValues, eventValue);
  };
  inline void PrintLogE(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    if (minLogLevel > 5) {return;}
    Write(5, "Error", eventName, keyValues, eventValue);
  }
  inline void PrintLogW(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    if (minLogLevel > 4) {return;}
    Write(4, "Warn", eventName, keyValues, eventValue);
  };
  inline void PrintLogI(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    if (minLogLevel > 2) {return;}
    Write(2, "Info", eventName, keyValues, eventValue);
  };
  inline void PrintLogD(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    if (minLogLevel > 1) {return;}
    Write(1, "Debug", eventName, keyValues, eventValue);
  }

  // debug == 1
  // info == 2
  // event == 3
  // warning == 4
  // error == 5
  inline void SetMinLogLevel(int logLevel) {minLogLevel = logLevel; };
  
  inline void SetCallback(LogCallback callback) { logCallback = callback; }
private:
  void Write(int logLevel, const char* logLevelName, const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue);

  std::mutex _mutex;
  int minLogLevel;
  LogCallback logCallback;
};

} // end namespace Cozmo
} // end namespace Anki


#endif //__CSharpBinding__unity_logger_provider_h_
