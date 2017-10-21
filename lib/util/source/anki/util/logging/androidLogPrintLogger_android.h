/**
* File: androidLogPrintLogger_android.h
*
* Description: Implements IFormattedLoggerProvider for __android_log_print()
*
* Copyright: Anki, inc. 2017
*
*/
#ifndef __Util_Logging_AndroidLogPrintLogger_H_
#define __Util_Logging_AndroidLogPrintLogger_H_

#include "util/logging/iFormattedLoggerProvider.h"

#include <string>

namespace Anki {
namespace Util {

class AndroidLogPrintLogger : public IFormattedLoggerProvider {
public:

  AndroidLogPrintLogger(const std::string& tag = "anki");
  
  // Implements IFormattedLoggerProvider
  virtual void Log(ILoggerProvider::LogLevel logLevel, const std::string& message) override;


private:
  std::string _tag;

}; // class AndroidLogPrintLogger

} // end namespace Util
} // end namespace Anki

#endif //__Util_Logging_AndroidLogPrintLogger_H_
