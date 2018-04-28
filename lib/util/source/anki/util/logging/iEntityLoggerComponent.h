/**
 * File: iEntityLoggerComponent.h
 *
 * Author: damjan
 * Created: 5/5/15.
 *
 * Description: Provides logging to single entity, logs comming out of
 * this logger will have entity data attached to the correct fields ($phys $data $group).
 *
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#ifndef __Util_iEntityLoggerComponent_H__
#define __Util_iEntityLoggerComponent_H__

namespace Anki {
namespace Util {

class IEntityLoggerComponent {
public:
  virtual ~IEntityLoggerComponent() {};
  // loggers
  virtual void ErrorF(const char* eventName, const char* eventValueFormat, ...) const __attribute((format(printf, 3, 4))) {};
  virtual void WarnF(const char *eventName, const char *eventValueFormat, ...) const __attribute((format(printf, 3, 4))) {};
  virtual void InfoF(const char* eventName, const char* eventValueFormat, ...) const __attribute((format(printf, 3, 4))) {};
  virtual void ChanneledInfoF(const char* channelName, const char* eventName, const char* eventValueFormat, ...) const __attribute((format(printf, 4, 5))) {};
  virtual void DebugF(const char* eventName, const char* eventValueFormat, ...) const __attribute((format(printf, 3, 4))) {};

  //__attribute__((__deprecated__))
  virtual void EventF(const char *eventName, const char *eventValueFormat, ...) const __attribute((format(printf, 3, 4))) {};
};


} // end namespace Util
} // end namespace Anki

#endif //__Util_iEntityLoggerComponent_H__
