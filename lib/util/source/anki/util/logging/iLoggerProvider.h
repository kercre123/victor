/**
* File: iLoggerProvider
*
* Author: damjan
* Created: 4/21/2015
*
* Description: Interface for classes that provide a different method of logging (console, network, file, ..)
*
* Copyright: Anki, Inc. 2014
*
**/

#ifndef __Util_Logging_ILoggerProvider_H__
#define __Util_Logging_ILoggerProvider_H__

#include "iChannelFilter.h"
#include <memory>
#include <vector>

namespace Anki {
namespace Util {

class ILoggerProvider {
public:

  // todo: this is duplicated all over anki code (DASLogLevel, errorHandling, ...). We should standarize
  enum LogLevel {
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_EVENT,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    
    _LOG_LEVEL_COUNT // control field. Do not use for regular logging
  };
  
  // constructor/destructor
  ILoggerProvider(){}
  virtual ~ILoggerProvider(){};
  
  // returns a descriptive string for the given log level / or level from string
  static const char* GetLogLevelString(LogLevel level);
  static LogLevel GetLogLevelValue(const std::string& levelStr);
  
  // set filter after creation. Note this provider will keep a shared_ptr to the filter
  void SetFilter(const std::shared_ptr<const IChannelFilter>& infoFilter);
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Unfiltered logs
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Channel filters don't apply to these levels

  virtual void PrintEvent(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;
  virtual void PrintLogE(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;
  virtual void PrintLogW(const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Filtered logs
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Channel filters apply to these levels. This method will filter messages and delegate to subclasses through
  // protected API the messages that pass the filter
  
  // Delegates on PrintLogI to print infos that pass channel filtering
  void PrintChanneledLogI(const char* channel,
    const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue);

  void PrintChanneledLogD(const char* channel,
                          const char* eventName,
                          const std::vector<std::pair<const char*, const char*>>& keyValues,
                          const char* eventValue);

  // Perform synchronous flush to underlying storage
  virtual void Flush() {};
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Internal API
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Required in subclasses to pring messages that pass their filter

  virtual void PrintLogI(const char* channel,
    const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;

  virtual void PrintLogD(const char* channel,
    const char* eventName,
    const std::vector<std::pair<const char*, const char*>>& keyValues,
    const char* eventValue) = 0;

private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // pointer to the filter to apply to infos (if any)
  std::shared_ptr<const IChannelFilter> _infoFilter;
};

} // namespace Util
} // namespace Anki

#endif // __Util_Logging_ILoggerProvider_H__

