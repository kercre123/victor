/**
* File: printfLoggerProvider
*
* Author: damjan stulic
* Created: 4/25/15
*
* Description: 
*
* Copyright: Anki, inc. 2015
*
*/

#include "util/logging/printfLoggerProvider.h"

namespace Anki {
namespace Util {


PrintfLoggerProvider::PrintfLoggerProvider()
: _minToStderrLevel(1000)
{
  SetMinLogLevel(LOG_LEVEL_INFO);
}

PrintfLoggerProvider::PrintfLoggerProvider(ILoggerProvider::LogLevel minToStderrLogLevel)
: _minToStderrLevel(minToStderrLogLevel)
{
  SetMinLogLevel(LOG_LEVEL_INFO);
}

void PrintfLoggerProvider::Log(ILoggerProvider::LogLevel logLevel, const std::string& message)
{
  FILE* outStream = _minToStderrLevel > logLevel ? stdout : stderr;
  fprintf(outStream, "%s",message.c_str());
  fflush(outStream);
}

} // end namespace Util
} // end namespace Anki
