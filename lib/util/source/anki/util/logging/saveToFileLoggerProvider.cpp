/**
* File: saveToFileLoggerProvider
*
* Author: Lee Crippen
* Created: 3/29/2016
*
* Description: 
*
* Copyright: Anki, inc. 2016
*
*/

#include "util/logging/saveToFileLoggerProvider.h"
#include "util/logging/rollingFileLogger.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include <fstream>
#include <assert.h>

namespace Anki {
namespace Util {
  
  
SaveToFileLoggerProvider::SaveToFileLoggerProvider(const std::string& baseDirectory, std::size_t maxFileSize)
: IFormattedLoggerProvider()
, _fileLogger(new RollingFileLogger(baseDirectory, RollingFileLogger::kDefaultFileExtension, maxFileSize))
{
  
}
  
SaveToFileLoggerProvider::~SaveToFileLoggerProvider() = default;
  
void SaveToFileLoggerProvider::Log(ILoggerProvider::LogLevel logLevel, const std::string& message)
{
  _fileLogger->Write(message);
}

} // end namespace Util
} // end namespace Anki
