//
//  util/console/consoleChannel.h
//
//  Created by Brian Chapados on 4/22/14.
//  Copyright (c) 2014-2018 Anki Inc. All rights reserved.
//

#ifndef ANKIUTIL_CONSOLE_CHANNEL_H
#define ANKIUTIL_CONSOLE_CHANNEL_H

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#ifdef __cplusplus

namespace Anki {
namespace Util {

class IConsoleChannel {
public:

  virtual ~IConsoleChannel() = default;

  // Returns true if channel is ready for I/O
  virtual bool IsOpen() = 0;

  virtual int WriteData(uint8_t *buffer, int len) = 0;

  virtual int WriteLog(const char *format, ...) __attribute__((format(printf,2,3))) = 0;
  virtual int WriteLogv(const char *format, va_list args) = 0;

  virtual bool Flush() = 0;

  virtual bool IsTTYLoggingEnabled() const = 0;
  virtual void SetTTYLoggingEnabled(bool newVal) = 0;

  virtual const char* GetChannelName() const = 0;
  virtual void SetChannelName(const char* newName) = 0;
};

} // end namespace Util
} // end namespace Anki

typedef Anki::Util::IConsoleChannel* ConsoleChannelRef;

#else /* ifdef __cplusplus */

// C interface: declare a struct pointer for function interface.
typedef struct IConsoleChannel* ConsoleChannelRef;

#endif /* ifdef __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif
  
extern bool  AnkiConsoleChannelIsOpen(ConsoleChannelRef channel);
extern int   AnkiConsoleChannelWriteData(ConsoleChannelRef channel, uint8_t *buffer, size_t len);
extern int   AnkiConsoleChannelWriteLog(ConsoleChannelRef channel, const char *format, ...);
  
#ifdef __cplusplus
}
#endif

#endif // #define ANKIUTIL_CONSOLE_CHANNEL_H
