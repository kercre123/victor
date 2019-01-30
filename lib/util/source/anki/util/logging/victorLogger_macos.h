/**
* File: util/logging/victorLogger_vicos.h
*
* Description: Implements ILoggerProvider for Victor
*
* Copyright: Anki, inc. 2018
*
*/
#ifndef __util_logging_victorLogger_macos_h
#define __util_logging_victorLogger_macos_h

#include "util/logging/logging.h"
#include "util/logging/iLoggerProvider.h"
#include "util/logging/iEventProvider.h"

#include <string>
#include <mutex>
#include <map>

namespace Anki {
namespace Util {

class VictorLogger : public ILoggerProvider, public IEventProvider {
public:

  VictorLogger(const std::string& tag = "anki");

  // Implements ILoggerProvider
  virtual void PrintLogE(const char * name, const KVPairVector & keyvals, const char * strval)
  {
    Log("ERROR", name, keyvals, strval);
  }

  virtual void PrintLogW(const char* name, const KVPairVector & keyvals, const char * strval)
  {
    Log("WARN", name, keyvals, strval);
  }

  virtual void PrintLogI(const char * channel, const char * name, const KVPairVector & keyvals, const char * strval)
  {
    Log("INFO", channel, name, keyvals, strval);
  }

  virtual void PrintLogD(const char * channel, const char * name, const KVPairVector & keyvals, const char * strval)
  {
    Log("DEBUG", channel, name, keyvals, strval);
  }

  virtual void PrintEvent(const char * name, const KVPairVector & keyvals, const char * strval)
  {
    LogEvent("INFO", name, keyvals);
  }

  // Implements IEventProvider
  virtual void SetGlobal(const char * key, const char * value);
  virtual void GetGlobals(std::map<std::string, std::string> & globals);

private:
  std::string _tag;
  std::mutex _mutex;
  std::map<std::string, std::string> _globals;

  void Log(const char * prio,
    const char * name,
    const KVPairVector & keyvals,
    const char * strval);

  void Log(const char * prio,
    const char * channel,
    const char * name,
    const KVPairVector & keyvals,
    const char * strval);

  void LogEvent(const char * prio,
    const char * name,
    const KVPairVector & keyvals);

  void LogEvent(LogLevel level, const DasMsg & dasMsg);

}; // class VictorLogger

} // end namespace Util
} // end namespace Anki

#endif //__util_logging_victorLogger_vicos_h
