/**
* File: iLoggerProvider.cpp
*
* Author: raul
* Created: 06/30/16
*
* Description: interface for anki log
*
* Copyright: Anki, Inc. 2014
*
**/
#include "iLoggerProvider.h"

#include "logging.h"

namespace Anki {
namespace Util {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ILoggerProvider::SetFilter(const std::shared_ptr<const IChannelFilter>& infoFilter)
{
  _infoFilter = infoFilter;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* ILoggerProvider::GetLogLevelString(LogLevel level)
{
  switch(level)
  {
    case LOG_LEVEL_DEBUG: { return "Debug"; }
    case LOG_LEVEL_INFO : { return "Info";  }
    case LOG_LEVEL_EVENT: { return "Event"; }
    case LOG_LEVEL_WARN : { return "Warn";  }
    case LOG_LEVEL_ERROR: { return "Error"; }
    case _LOG_LEVEL_COUNT: {
      
    }
  };

  ASSERT_NAMED_EVENT(false, "ILoggerProvider.GetLogLevelString.InvalidLogLevel", "%d is not a valid level", level);
  return "Invalid_Log_Level!";
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ILoggerProvider::LogLevel ILoggerProvider::GetLogLevelValue(const std::string& levelStr)
{
  std::string levelLC = levelStr;
  std::transform(levelLC.begin(), levelLC.end(), levelLC.begin(), ::tolower);
  if ( levelLC == "debug" ) {
    return LOG_LEVEL_DEBUG;
  } else if ( levelLC == "info" ) {
    return LOG_LEVEL_INFO;
  } else if ( levelLC == "event" ) {
    return LOG_LEVEL_EVENT;
  } else if ( levelLC == "warn" ) {
    return LOG_LEVEL_WARN;
  } else if ( levelLC == "error" ) {
    return LOG_LEVEL_ERROR;
  }
  
  ASSERT_NAMED_EVENT(false, "ILoggerProvider.GetLogLevelValue.InvalidLogLevel", "'%s' is not a valid level", levelStr.c_str());
  return _LOG_LEVEL_COUNT;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ILoggerProvider::PrintChanneledLogI(const char* channel,
      const char* eventName,
      const std::vector<std::pair<const char*, const char*>>& keyValues,
      const char* eventValue)
{
  // if no filter is set or the channel is enabled
  if ( !_infoFilter || _infoFilter->IsChannelEnabled(channel) )
  {
    // pass to subclass
    PrintLogI(channel, eventName, keyValues, eventValue);
  }
}

void ILoggerProvider::PrintChanneledLogD(const char* channel,
      const char* eventName,
      const std::vector<std::pair<const char*, const char*>>& keyValues,
      const char* eventValue)
{
  // if no filter is set or the channel is enabled
  if ( !_infoFilter || _infoFilter->IsChannelEnabled(channel) )
  {
    // pass to subclass
    PrintLogD(channel, eventName, keyValues, eventValue);
  }
}

} // namespace Util
} // namespace Anki
