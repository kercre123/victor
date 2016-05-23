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
bool _errG = false;
const size_t kMaxStringBufferSize = 1024;



void sEventF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...)
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

void sEventV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args)
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

void sEvent(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  gLoggerProvider->PrintEvent(eventName, keyValues, eventValue);
}



void sErrorF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  va_start(args, format);
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  va_end(args);
  gLoggerProvider->PrintLogE(eventName, keyValues, logString);
}

void sErrorV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  gLoggerProvider->PrintLogE(eventName, keyValues, logString);
}

void sError(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  if (gTickTimeProvider != nullptr) {
    char logString[kMaxStringBufferSize]{0};
    snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : %s", gTickTimeProvider->GetTickCount(), eventValue);
    gLoggerProvider->PrintLogE(eventName, keyValues, logString);
  } else {
    gLoggerProvider->PrintLogE(eventName, keyValues, eventValue);
  }
}




void sWarningF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  va_start(args, format);
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  va_end(args);
  gLoggerProvider->PrintLogW(eventName, keyValues, logString);
}

void sWarningV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  gLoggerProvider->PrintLogW(eventName, keyValues, logString);
}

void sWarning(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  if (gTickTimeProvider != nullptr) {
    char logString[kMaxStringBufferSize]{0};
    snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : %s", gTickTimeProvider->GetTickCount(), eventValue);
    gLoggerProvider->PrintLogW(eventName, keyValues, logString);
  } else {
    gLoggerProvider->PrintLogW(eventName, keyValues, eventValue);
  }
}




void sInfoF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  va_start(args, format);
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  va_end(args);
  gLoggerProvider->PrintLogI(eventName, keyValues, logString);
}
  
void sChanneledInfoF(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  std::string channelNameString(channelName);
  if(gChannelFilter.IsInitialized()) {
    if(!gChannelFilter.IsChannelRegistered(channelNameString)) {
      PRINT_NAMED_ERROR("UnregisteredChannel", "Channel @%s not registered!", channelName);
    } else {
      if (!gChannelFilter.IsChannelEnabled(channelNameString)) {
        return;
      }
    }
    offset += snprintf(logString, kMaxStringBufferSize, "[@%s] ", channelName);
  }
  
  if (gTickTimeProvider != nullptr) {
    offset += snprintf(&logString[offset], kMaxStringBufferSize - offset, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  va_start(args, format);
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  va_end(args);
  gLoggerProvider->PrintLogI(eventName, keyValues, logString);
}

void sInfoV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  gLoggerProvider->PrintLogI(eventName, keyValues, logString);
}

void sChanneledInfoV(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  std::string channelNameString(channelName);
  if(gChannelFilter.IsInitialized()) {
    if(!gChannelFilter.IsChannelRegistered(channelNameString)) {
      PRINT_NAMED_ERROR("UnregisteredChannel", "Channel @%s not registered!", channelName);
    } else {
      if (!gChannelFilter.IsChannelEnabled(channelNameString)) {
        return;
      }
    }
    offset += snprintf(logString, kMaxStringBufferSize, "[@%s] ", channelName);
  }
  if (gTickTimeProvider != nullptr) {
    offset += snprintf(&logString[offset], kMaxStringBufferSize - offset, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  gLoggerProvider->PrintLogI(eventName, keyValues, logString);
}

void sInfo(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  if (gTickTimeProvider != nullptr) {
    char tickLogString[kMaxStringBufferSize]{0};
    snprintf(tickLogString, kMaxStringBufferSize, "(tc%04zd) : %s", gTickTimeProvider->GetTickCount(), eventValue);
    gLoggerProvider->PrintLogI(eventName, keyValues, tickLogString);
  } else {
    gLoggerProvider->PrintLogI(eventName, keyValues, eventValue);
  }
}




void sDebugF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  va_start(args, format);
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  va_end(args);
  gLoggerProvider->PrintLogD(eventName, keyValues, logString);
}

void sDebugV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  char logString[kMaxStringBufferSize]{0};
  int offset = 0;
  if (gTickTimeProvider != nullptr) {
    offset = snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : ", gTickTimeProvider->GetTickCount());
  }
  vsnprintf(&logString[offset], kMaxStringBufferSize - offset, format, args);
  gLoggerProvider->PrintLogD(eventName, keyValues, logString);
}

void sDebug(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue)
{
  if (gLoggerProvider == nullptr) {
    return;
  }
  if (gTickTimeProvider != nullptr) {
    char logString[kMaxStringBufferSize]{0};
    snprintf(logString, kMaxStringBufferSize, "(tc%04zd) : %s", gTickTimeProvider->GetTickCount(), eventValue);
    gLoggerProvider->PrintLogD(eventName, keyValues, logString);
  } else {
    gLoggerProvider->PrintLogD(eventName, keyValues, eventValue);
  }
}



void sSetGlobal(const char* key, const char* value)
{
  if (gEventProvider == nullptr) {
    return;
  }
  gEventProvider->SetGlobal(key, value);
}

} // namespace Util
} // namespace Anki


