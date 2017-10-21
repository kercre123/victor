/**
* File: androidLogPrintLogger_android.cpp
*
* Description: Implements IFormattedLoggerProvider for __android_log_print()
*
* Copyright: Anki, inc. 2017
*
*/

#include "util/logging/androidLogPrintLogger_android.h"

#include <android/log.h>

namespace Anki {
namespace Util {

AndroidLogPrintLogger::AndroidLogPrintLogger(const std::string& tag) :
  _tag(tag)
{

}

void AndroidLogPrintLogger::Log(ILoggerProvider::LogLevel level, const std::string& message)
{
  android_LogPriority priority = ANDROID_LOG_DEFAULT;
  
  switch (level) {
    case LogLevel::LOG_LEVEL_DEBUG:
      priority = ANDROID_LOG_DEBUG;
    break;
    case LogLevel::LOG_LEVEL_INFO:
    case LogLevel::LOG_LEVEL_EVENT:
      priority = ANDROID_LOG_INFO;
    break;
    case LogLevel::LOG_LEVEL_WARN:
      priority = ANDROID_LOG_WARN;
      break;
    case LogLevel::LOG_LEVEL_ERROR:
      priority = ANDROID_LOG_ERROR;
      break;
    default:
      // should never be here
      break;
  }
  
  __android_log_print(priority, _tag.c_str(), "%s", message.c_str());
}


} // end namespace Util
} // end namespace Anki
