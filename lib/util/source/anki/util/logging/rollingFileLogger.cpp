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

#include "util/logging/rollingFileLogger.h"
#include "util/dispatchQueue/dispatchQueue.h"
#include "util/fileUtils/fileUtils.h"
#include "assert.h"

#include <sstream>
#include <iomanip>

#include <ctime>
#include <time.h> // Needed for the POSIX thread-safe version of local_time and put_time

// This alternative error logging define imported from our DAS library implementation
#define LOGD(...) {fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}

namespace Anki {
namespace Util {
  
const char * const RollingFileLogger::kDefaultFileExtension = ".log";
  
RollingFileLogger::RollingFileLogger(const std::string& baseDirectory, const std::string& extension, std::size_t maxFileSize)
: _dispatchQueue(Dispatch::Create())
, _baseDirectory(baseDirectory)
, _extension(extension)
, _maxFileSize(maxFileSize)
{
  FileUtils::CreateDirectory(baseDirectory);
}
  
RollingFileLogger::~RollingFileLogger()
{
  Dispatch::Stop(_dispatchQueue);
  Dispatch::Release(_dispatchQueue);
  _currentLogFileHandle.close();
}

void RollingFileLogger::Write(const std::string& message)
{
  Dispatch::Async(_dispatchQueue, [this, message] {
    WriteInternal(message);
  });
}
  
// Note that WriteInternal and everything it calls will occur on another thread.
void RollingFileLogger::WriteInternal(const std::string& message)
{
  auto messageSize = message.length();
  // If file handle is closed or we've run out of space, open a new one
  if (!_currentLogFileHandle.is_open() || (messageSize + _numBytesWritten) > _maxFileSize)
  {
    RollLogFile();
    _numBytesWritten = 0;
  }
  
  assert(_currentLogFileHandle);
  
  if(!_currentLogFileHandle.is_open())
  {
    return;
  }
  
  _currentLogFileHandle << message;
  _currentLogFileHandle.flush();
  _numBytesWritten += messageSize;
}
  
void RollingFileLogger::RollLogFile()
{
  if (_currentLogFileHandle.is_open())
  {
    _currentLogFileHandle.close();
  }
  std::string nextFilename = GetNextFileName();
  _currentLogFileHandle.open(nextFilename, std::ofstream::out | std::ofstream::app);
  
  if (!_currentLogFileHandle)
  {
    LOGD("Error getting handle for file %s: %s !!", nextFilename.c_str(), strerror(errno));
  }
}

std::string RollingFileLogger::GetNextFileName()
{
  std::ostringstream pathStream;
  if (!_baseDirectory.empty())
  {
    pathStream << _baseDirectory << '/';
  }
  
  pathStream << GetDateTimeString(std::chrono::system_clock::now()) << _extension;
  return pathStream.str();
}
  
std::string RollingFileLogger::GetDateTimeString(const std::chrono::system_clock::time_point& time)
{
  std::ostringstream stringStream;
  auto currTime_t = std::chrono::system_clock::to_time_t(time);
  auto numSecs = std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch());
  auto millisLeft = std::chrono::duration_cast<std::chrono::milliseconds>((time - numSecs).time_since_epoch());
  
  struct tm localTime; // This is local scoped to make it thread safe
  localtime_r(&currTime_t, &localTime);
  
  // Use the old fashioned strftime for thread safety, instead of std::put_time
  char formatTimeBuffer[256];
  strftime(formatTimeBuffer, sizeof(formatTimeBuffer), "%FT%H-%M-%S-", &localTime);
  
  stringStream << formatTimeBuffer << std::setfill('0') << std::setw(3) << millisLeft.count();
  return stringStream.str();
}

} // end namespace Util
} // end namespace Anki
