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
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <sstream>

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
IEventProvider* gEventProvider = nullptr;
bool _errG = false;
const size_t kMaxStringBufferSize = 1024;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// helpers
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
using KVV = std::vector<std::pair<const char*, const char*>>;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogError(const char* eventName, const KVV& keyValues, const char* string)
{
  if (gLoggerProvider == nullptr) {
    return;
  }

  // set tick count if available
  std::stringstream finalLogStr;
  if ( gTickTimeProvider ) {
    finalLogStr << "(tc";
    finalLogStr << std::right << std::setw(4) << std::setfill('0') << gTickTimeProvider->GetTickCount();
    finalLogStr << ") : ";
  }
 
  // append string to tick count
  finalLogStr << string;
  gLoggerProvider->PrintLogE(eventName, keyValues, finalLogStr.str().c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogWarning(const char* eventName, const KVV& keyValues, const char* string)
{
  if (gLoggerProvider == nullptr) {
    return;
  }

  // set tick count if available
  std::stringstream finalLogStr;
  if ( gTickTimeProvider ) {
    finalLogStr << "(tc";
    finalLogStr << std::right << std::setw(4) << std::setfill('0') << gTickTimeProvider->GetTickCount();
    finalLogStr << ") : ";
  }
 
  // append string to tick count
  finalLogStr << string;
  gLoggerProvider->PrintLogW(eventName, keyValues, finalLogStr.str().c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogChanneledInfo(const char* channel, const char* eventName, const KVV& keyValues, const char* string)
{
  if (gLoggerProvider == nullptr) {
    return;
  }

  // set tick count if available
  std::stringstream finalLogStr;
  if ( gTickTimeProvider ) {
    finalLogStr << "(tc";
    finalLogStr << std::right << std::setw(4) << std::setfill('0') << gTickTimeProvider->GetTickCount();
    finalLogStr << ") : ";
  }
 
  // append string to tick count
  finalLogStr << string;
  gLoggerProvider->PrintChanneledLogI(channel, eventName, keyValues, finalLogStr.str().c_str());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void LogChannelDebug(const char* channel, const char* eventName, const KVV& keyValues, const char* string)
{
  if (gLoggerProvider == nullptr) {
    return;
  }

  // set tick count if available
  std::stringstream finalLogStr;
  if ( gTickTimeProvider ) {
    finalLogStr << "(tc";
    finalLogStr << std::right << std::setw(4) << std::setfill('0') << gTickTimeProvider->GetTickCount();
    finalLogStr << ") : ";
  }
 
  // append string to tick count
  finalLogStr << string;
  gLoggerProvider->PrintChanneledLogD(channel, eventName, keyValues, finalLogStr.str().c_str());
}

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sEventF(const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
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
  if (gLoggerProvider == nullptr) {
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
  if (gLoggerProvider == nullptr) {
    return;
  }
  gLoggerProvider->PrintEvent(eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sErrorF(const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
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
  if (gLoggerProvider == nullptr) {
    return;
  }

  // parse string
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);

  // log it
  LogError(eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sError(const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
    return;
  }

  // log it
  LogError(eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sWarningF(const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
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
  if (gLoggerProvider == nullptr) {
    return;
  }

  // parse string
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  
  // log it
  LogWarning(eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sWarning(const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
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
  if (gLoggerProvider == nullptr) {
    return;
  }
  
  // parse string
  char logString[kMaxStringBufferSize]{0};
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  
  // log it
  LogChanneledInfo(channelName, eventName, keyValues, logString);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sChanneledInfo(const char* channelName, const char* eventName, const KVV& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  
  // log it
  LogChanneledInfo(channelName, eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sChanneledDebugF(const char* channelName, const char* eventName, const KVV& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
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
  if (gLoggerProvider == nullptr) {
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
  if (gLoggerProvider == nullptr) {
    return;
  }
  
  // log it
  LogChannelDebug(channelName, eventName, keyValues, eventValue);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void sSetGlobal(const char* key, const char* value)
{
  if (gEventProvider == nullptr) {
    return;
  }
  gEventProvider->SetGlobal(key, value);
}

} // namespace Util
} // namespace Anki


