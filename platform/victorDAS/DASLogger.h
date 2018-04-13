/**
 * File: platform/victorDAS/DASLogger.h
 *
 * Description: Interface between DAS and Anki::Util::Logging
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __victorDAS_DASLogger_h
#define __victorDAS_DASLogger_h

#include "util/logging/iLoggerProvider.h"
#include "util/logging/iEventProvider.h"
#include "DAS/DAS.h"

namespace Anki {
namespace VictorDAS {

class DASLogger :
  public Anki::Util::ILoggerProvider,
  public Anki::Util::IEventProvider
{
public:
  DASLogger() = default;
  virtual ~DASLogger() = default;

protected:

  // ILoggerProvider methods

  inline void PrintEvent(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    _DAS_LogKv(DASLogLevel_Event, eventName, eventValue, keyValues);
  }

  inline void PrintLogE(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    _DAS_LogKv(DASLogLevel_Error, eventName, eventValue, keyValues);
  }

  inline void PrintLogW(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    _DAS_LogKv(DASLogLevel_Warn, eventName, eventValue, keyValues);
  }

  inline void PrintLogI(const char* channel,
    const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    // note: ignoring channel in this provider
    _DAS_LogKv(DASLogLevel_Info, eventName, eventValue, keyValues);
  }

  inline void PrintLogD(const char* channel,
    const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) override {
    _DAS_LogKv(DASLogLevel_Debug, eventName, eventValue, keyValues);
  }

  // IEventProvider methods

  inline void SetGlobal(const char* key, const char* value) override {
    _DAS_SetGlobal(key, value);
  }

  inline void GetGlobals(std::map<std::string, std::string>& dasGlobals) override {
    _DAS_GetGlobalsForThisRun(dasGlobals);
  }

};

} // end namespace VictorDAS
} // end namespace Anki

#endif
