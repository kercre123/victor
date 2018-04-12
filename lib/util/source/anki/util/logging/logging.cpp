/**
 * File: logging
 *
 * Author: damjan
 * Created: 4/3/2014
 *
 * Description: logging functions.
 * structure of the function names is: s<Level><style>
 *   levels are: Event, Error, Warning, Info, Debug
 *   style: f - takes (...) , v - takes va_list
 *   functions are spelled out, instead of stacked (sErrorF -> calls sErrorV -> calls sError)
 *   to improve on the stack space. If you think about improving on this, please consider macros to re-use code.
 *   If you however feel that we should stack them into one set of function that uses LogLevel as a param, think about need
 *   to translate Ank::Util::LogLevel to DasLogLevel and then to ios/android LogLevel.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#include "util/logging/logging.h"
#include "util/logging/iTickTimeProvider.h"
#include "util/logging/iLoggerProvider.h"
#include "util/logging/channelFilter.h"
#include "util/logging/iEventProvider.h"
#include "util/helpers/ankiDefines.h"

#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <signal.h>

namespace Anki{
namespace Util {

std::string HexDump(const void *value, const size_t len, char delimiter)
{
  const unsigned char *bytes = (const unsigned char *) value;
  size_t bufferLen = len * 3;
  char *str = (char *) malloc(sizeof (char) * bufferLen);
  memset(str, 0, bufferLen);

  const char *hex = "0123456789ABCDEF";
  char *s = str;

  for (size_t i = 0; i < len - 1; ++i) {
    *s++ = hex[(*bytes >> 4)&0xF];
    *s++ = hex[(*bytes++)&0xF];
    *s++ = delimiter;
  }
  *s++ = hex[(*bytes >> 4)&0xF];
  *s++ = hex[(*bytes++)&0xF];
  *s++ = 0;

  std::string cppString(str);
  free(str);
  return cppString;
}

ITickTimeProvider* gTickTimeProvider = nullptr;
ILoggerProvider*gLoggerProvider = nullptr;
ChannelFilter gChannelFilter;
IEventProvider* gEventProvider = nullptr;

// Has an error been reported?
bool _errG = false;
  
// Do we break on any error?
bool _errBreakOnError = true;


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
using KVV = std::vector<std::pair<const char*, const char*>>;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void AddTickCount(std::ostringstream& oss)
{
  if ( gTickTimeProvider ) {
    oss << "(tc";
    oss << std::right << std::setw(4) << std::setfill('0') << gTickTimeProvider->GetTickCount();
    oss << ") ";
  }
  oss << ": ";
}

std::string PrependTickCount(const char* logString)
{
  std::ostringstream oss;
  AddTickCount(oss);
  oss << logString;

  return std::string(oss.str());
}

void LogError(const char* eventName, const KVV& keyValues, const char* logString)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  gLoggerProvider->PrintLogE(eventName, keyValues, PrependTickCount(logString).c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogWarning(const char* eventName, const KVV& keyValues, const char* logString)
{
  if (gLoggerProvider == nullptr) {
    return;
  }

  gLoggerProvider->PrintLogW(eventName, keyValues, PrependTickCount(logString).c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogChanneledInfo(const char* channel, const char* eventName, const KVV& keyValues, const char* logString)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  // set tick count and channel name if available
  std::ostringstream finalLogStr;
  AddTickCount(finalLogStr);

  std::string channelNameString(channel);
  if(gChannelFilter.IsInitialized()) {
    if(!gChannelFilter.IsChannelRegistered(channelNameString)) {
      PRINT_NAMED_ERROR("UnregisteredChannel", "Channel @%s not registered!", channel);
    } else {
      if (!gChannelFilter.IsChannelEnabled(channelNameString)) {
        return;
      }
    }
    finalLogStr << "[@";
    finalLogStr << channel;
    finalLogStr << "] ";
  }
  finalLogStr << logString;

  gLoggerProvider->PrintChanneledLogI(channel, eventName, keyValues, finalLogStr.str().c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogChannelDebug(const char* channel, const char* eventName, const KVV& keyValues, const char* logString)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  gLoggerProvider->PrintChanneledLogD(channel, eventName, keyValues, PrependTickCount(logString).c_str());
}

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void DasMsg::formatEventValue(const char* format, va_list args)
{
  vsnprintf(_eventValue, kMaxStringBufferSize, format, args);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sEventF(const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  // event is BI event, and the data is specifically formatted to be read on the backend.
  // we should not modify tis data under any circumstance. Hence, no tick timer here
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  va_start(args, format);
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  va_end(args);
  gLoggerProvider->PrintEvent(eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sEventV(const char* eventName, const KVV& keyValues, const char* format, va_list args)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  // event is BI event, and the data is specifically formatted to be read on the backend.
  // we should not modify tis data under any circumstance. Hence, no tick timer here
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  gLoggerProvider->PrintEvent(eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sEvent(const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  gLoggerProvider->PrintEvent(eventName, keyValues, eventValue);
}

void sEventD(DasMsg& dasMessage)
{
  sEvent(dasMessage.eventName(), {}, dasMessage._eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sErrorF(const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  // parse string
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  va_start(args, format);
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  va_end(args);

  // log it
  LogError(eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sErrorV(const char* eventName, const KVV& keyValues, const char* format, va_list args)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  // parse string
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);

  // log it
  LogError(eventName, keyValues, logString);
}

void sErrorD(DasMsg& dasMessage)
{
  sError(dasMessage.eventName(), {}, dasMessage._eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sError(const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  // log it
  LogError(eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sWarningF(const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  // parse string
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  va_start(args, format);
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  va_end(args);

  // log it
  LogWarning(eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sWarningV(const char* eventName, const KVV& keyValues, const char* format, va_list args)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  // parse string
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  
  // log it
  LogWarning(eventName, keyValues, logString);
}

void sWarningD(DasMsg& dasMessage)
{
  sWarning(dasMessage.eventName(), {}, dasMessage._eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sWarning(const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (nullptr == gLoggerProvider) {
    return;
  }

  // log it
  LogWarning(eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
void sChanneledInfoF(const char* channelName, const char* eventName, const KVV& keyValues, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  sChanneledInfoV(channelName, eventName, keyValues, format, args);
  va_end(args);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sChanneledInfoV(const char* channelName, const char* eventName, const KVV& keyValues, const char* format, va_list args)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  
  // parse string
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  
  // log it
  LogChanneledInfo(channelName, eventName, keyValues, logString);
}

void sChanneledInfoD(const char* channelName, DasMsg& dasMessage)
{
  sChanneledInfo(channelName, dasMessage.eventName(), {}, dasMessage._eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sChanneledInfo(const char* channelName, const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  
  // log it
  LogChanneledInfo(channelName, eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sChanneledDebugF(const char* channelName, const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  
  // parse string
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  va_start(args, format);
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  va_end(args);

  // log it
  LogChannelDebug(channelName, eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sChanneledDebugV(const char* channelName, const char* eventName, const KVV& keyValues, const char* format, va_list args)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  
  // parse string
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);

  // log it
  LogChannelDebug(channelName, eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sChanneledDebug(const char* channelName, const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  
  // log it
  LogChannelDebug(channelName, eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool sVerifyFailedReturnFalse(const char* eventName, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  sErrorV(eventName, {}, format, args);
  va_end(args);
  _errG=true;
  sDumpCallstack("VERIFY"); 
  sLogFlush();
  sDebugBreak();
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sLogFlush()
{
  if (nullptr == gLoggerProvider) {
    return;
  }
  gLoggerProvider->Flush();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void sSetGlobal(const char* key, const char* value)
{
  if (nullptr == gEventProvider) {
    return;
  }
  gEventProvider->SetGlobal(key, value);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void sDebugBreak()
{
  
#if ANKI_DEVELOPER_CODE
  
#if defined(ANKI_PLATFORM_IOS)
  
  // iOS device - break to supervisor process
  // This works on a debug build, but causes an access exception (EXC_BAD_ACCESS)
  // in a release build.
  asm volatile ("svc #0");
  
#elif defined(ANKI_PLATFORM_OSX)
  
  // MacOS X - break to supervisor process
  // This works for debug or release, but causes SIGTRAP if there is no supervisor.
  // http://stackoverflow.com/questions/37299/xcode-equivalent-of-asm-int-3-debugbreak-halt
  // asm volatile ("int $3");
  
  // Interrupt thread with no-op signal.  This causes debugger breakpoint inside pthread_kill.
  pthread_kill(pthread_self(), SIGCONT);
  
#else
  
  // Android, Windows, linux TBD
  // Send no-op signal to cause debugger break
  pthread_kill(pthread_self(), SIGCONT);
  
#endif // TARGET_OS
  
#endif // ANKI_DEVELOPER_CODE

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#ifndef ALLOW_BREAK_ON_ERROR
#define ALLOW_BREAK_ON_ERROR 1
#endif

void sDebugBreakOnError()
{
  #if ALLOW_BREAK_ON_ERROR
  sDebugBreak();
  #endif
}

#undef ALLOW_BREAK_ON_ERROR

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void sAbort()
{
  LogError("Util.Logging.Abort", {}, "Application abort");
  
  // Add breakpoint here to inspect application state */
  abort();
  
}

} // namespace Util
} // namespace Anki


