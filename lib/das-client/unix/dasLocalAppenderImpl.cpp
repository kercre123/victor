/**
 * File: dasLocalAppenderImpl
 *
 * Author: seichert
 * Created: 01/15/2015
 *
 * Description: DAS Local Appender for Unix (goes to stdout)
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "dasLocalAppenderImpl.h"
#include "dasLogMacros.h"
#include "DASPrivate.h"
#include "portableTypes.h"
#include <cstdint>
#include <ctime>
#include <atomic>
#include <chrono>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>


static std::atomic<uint64_t> sequence_number{0};

namespace Anki
{
namespace Das
{

void DasLocalAppenderImpl::append(DASLogLevel level, const char* eventName, const char* eventValue,
                              ThreadId_t threadId, const char* file, const char* funct, int line,
                              const std::map<std::string,std::string>* globals,
                              const std::map<std::string,std::string>& data,
                              const char* globalsAndDataInfo) {
  
  uint64_t curSequenceNumber = ++sequence_number;
  std::string timeStr;
  std::string logLevelName;
  getDASLogLevelName(level, logLevelName);
  getDASTimeString(timeStr);
  fprintf(stdout, "%" PRIu64 " (t:%02u) [%s] %s - %s = %s %s\n",
          curSequenceNumber, threadId, timeStr.c_str(), logLevelName.c_str(), eventName,
          eventValue, globalsAndDataInfo);
}

void DasLocalAppenderImpl::flush() {
  fflush(stdout);
}

} // namespace Das
} // namespace Anki
