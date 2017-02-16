/**
 * File: dasLocalAppenderImpl
 *
 * Author: shpung
 * Created: 05/05/14
 *
 * Description: DAS Local Appender for Android (goes to logcat)
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "dasLocalAppenderImpl.h"
#include "dasLogMacros.h"
#include <cstdio>
#include <android/log.h>

static const char* kLogTag = "das";
#define PRINT_TID 0

namespace Anki
{
namespace Das
{

void DasLocalAppenderImpl::append(DASLogLevel level, const char* eventName, const char* eventValue,
                              ThreadId_t threadId, const char* file, const char* funct, int line,
                              const std::map<std::string,std::string>* globals,
                              const std::map<std::string,std::string>& data,
                              const char* globalsAndDataInfo) {

  android_LogPriority priority = ANDROID_LOG_DEFAULT;
  char tidInfo[128] = {'\0'};

  switch (level) {
  case DASLogLevel_Debug:
    priority = ANDROID_LOG_DEBUG;
    break;
  case DASLogLevel_Info:
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
    // should never be here
    break;
  }

#if (PRINT_TID)
  // android injects thread ids natively, no need to print ours
  std::snprintf(tidInfoBuf, sizeof(tidInfoBuf), "(t:%02u) ", threadId);
#endif

  __android_log_print(priority, kLogTag, "%s%s : %s %s", tidInfo,
                      eventName, eventValue, globalsAndDataInfo);
}

void DasLocalAppenderImpl::flush() {
  fflush(stdout);
}

} // namespace Das
} // namespace Anki
