/**
 * File: dasLocalAppender
 *
 * Author: seichert
 * Created: 01/15/2015
 *
 * Description: DAS Local Appender for iOS (goes to stdout and /var/log/system.log)
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "dasLocalAppender.h"
#include "dasLogMacros.h"
#include "DASPrivate.h"
#include <cstdint>
#include <ctime>
#include <atomic>
#include <chrono>
#include <asl.h>
#include <sstream>
#include <iomanip>
#include <dispatch/dispatch.h>

static std::atomic<uint64_t> sequence_number{0};
static dispatch_queue_t _loggingQueue = NULL;

namespace Anki
{
namespace Das
{

void DasLocalAppender::append(DASLogLevel level, const char* eventName, const char* eventValue,
                              ThreadId_t threadId, const char* file, const char* funct, int line,
                              const std::map<std::string,std::string>* globals,
                              const std::map<std::string,std::string>& data,
                              const char* globalsAndDataInfo) {
  
  uint64_t curSequenceNumber = ++sequence_number;
  std::string logLevelName;
  getDASLogLevelName(level, logLevelName);
  std::string timeStr;
  getDASTimeString(timeStr);

  // Standard message looks like
  // 387 (t:01) [09:46:18:112] event : device.language_locale : en

  // Figure out the log message string here before handing it off for stdout and ASL logging
  // Since we are handing off ASL logging to a single thread which happens after this
  // function is done we need a full copy of the string since we can't depend on the pointers
  // to be valid
  std::stringstream logMsgStream;
  logMsgStream << curSequenceNumber << " (t:";
  logMsgStream << std::setfill('0') << std::setw(2) << threadId << ") [";
  logMsgStream << timeStr << "] ";
  logMsgStream << logLevelName << " : ";
  logMsgStream << eventName << " : ";
  logMsgStream << eventValue << " ";
  logMsgStream << globalsAndDataInfo << std::endl;

  std::string logMsg = logMsgStream.str();

  fprintf(stdout, "%s", logMsg.c_str());

  // [COZMO-2152] In addition to stdout also log to syslog so we can get the information after the fact
  // Do note LOG_DEBUG for this definition
  int priority = ASL_LEVEL_DEBUG;

  // According to /etc/asl.conf anything below a notice will not be logged to system.log
  // We don't have access to this file so we pretend that everything is a notice and use
  // the actual message to see what log level this is
  switch (level) {
    case DASLogLevel_Debug:
    case DASLogLevel_Info:
    case DASLogLevel_Event:
        priority = ASL_LEVEL_NOTICE;
        break;
    case DASLogLevel_Warn:
        priority = ASL_LEVEL_WARNING;
        break;
    case DASLogLevel_Error:
        priority = ASL_LEVEL_ERR;
        break;
    default:
        // should never be here
        break;
  }

  if (_loggingQueue == NULL) {
    _loggingQueue = dispatch_queue_create("com.anki.overdrive.iosLoggingQueue", NULL);
  }

  dispatch_async(_loggingQueue, ^{
    asl_log(NULL, NULL, priority, "%s", logMsg.c_str());
  });
}

} // namespace Das
} // namespace Anki
