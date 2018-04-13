/**
 * File: dasLocalAppenderImpl
 *
 * Description: DAS Local Appender for Android (goes to logcat)
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "dasLocalAppenderImpl.h"
#include "dasLogMacros.h"
#include <cstdio>
#include <sstream>
#include <assert.h>
#include <android/log.h>

// Use a magic log tag that gets redirected to radio buffer.
// The Android NDK does not expose any methods to address LOG_ID_RADIO directly.
static const char * kLogTag = "RIL-DAS";

#define PRINT_TID 0

namespace Anki {
namespace Das {

void DasLocalAppenderImpl::append(DASLogLevel level, const char* eventName, const char* eventValue,
                              ThreadId_t threadId, const char* file, const char* funct, int line,
                              const std::map<std::string,std::string>* globals,
                              const std::map<std::string,std::string>& data,
                              const char* globalsAndDataInfo) {

  assert(level >= DASLogLevel_Debug);
  assert(level <= DASLogLevel_Error);
  assert(eventName != nullptr);
  assert(eventValue != nullptr);
  assert(globals != nullptr);

  android_LogPriority priority = ANDROID_LOG_DEFAULT;

  switch (level) {
  case DASLogLevel_Debug:
    priority = ANDROID_LOG_VERBOSE;
    break;
  case DASLogLevel_Info:
    priority = ANDROID_LOG_DEBUG;
    break;
  case DASLogLevel_Event:
    priority = ANDROID_LOG_INFO;
    break;
  case DASLogLevel_Warn:
    priority = ANDROID_LOG_WARN;
    break;
  case DASLogLevel_Error:
    priority = ANDROID_LOG_ERROR;
    break;
  default:
    break;
  }

#if (PRINT_TID)
  // android injects thread ids natively, no need to print ours
  char tidInfo[128] = {'\0'};
  std::snprintf(tidInfoBuf, sizeof(tidInfoBuf), "(t:%02u) ", threadId);
#endif

  // Format log fields into compact pseudo-json format
  // TO DO: Escape embedded values or use actual JSON formatter
  // Note default JsonWriter adds embedded newline.
  std::ostringstream ostr;
  ostr << "{";
  ostr << "$name" << ':' << eventName << ',';
  ostr << "$value" << ':' << eventValue << ',';
  for (auto pos = globals->begin(); pos != globals->end(); ++pos) {
    ostr << pos->first << ':' << pos->second << ',';
  }
  for (auto pos = data.begin(); pos != data.end(); ++pos) {
    ostr << pos->first << ':' << pos->second << ',';
  }
  ostr << "}";

  // Text longer than 1024 chars will be silently truncated
  const std::string & str = ostr.str();
  assert(str.length() < 1024);

  // Log encoded struct to android log buffer.
  // Android NDK headers do not expose alternate log buffers,
  // but this could be an option with VicOS Linux toolchain.

  // If we don't filter logs at the application level, each log
  // message will be written to logcat once as plain text, then
  // again as an encoded event.
  __android_log_print(priority, kLogTag, "%s", str.c_str());

}

void DasLocalAppenderImpl::flush() {
  // No android log flush
}

} // namespace Das
} // namespace Anki
