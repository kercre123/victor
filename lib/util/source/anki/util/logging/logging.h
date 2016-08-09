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


#ifndef __Util_Logging_Logging_H_
#define __Util_Logging_Logging_H_

#include "util/logging/eventKeys.h"
#include "util/logging/callstack.h"
#include <string>
#include <vector>


namespace Anki{
namespace Util {

class ITickTimeProvider;
class ILoggerProvider;
class ChannelFilter;
class IEventProvider;

std::string HexDump(const void *value, const size_t len, char delimiter);

extern ITickTimeProvider* gTickTimeProvider;
extern ILoggerProvider* gLoggerProvider;
extern ChannelFilter gChannelFilter;
extern IEventProvider* gEventProvider;

// global error flag so we can check if PRINT_ERROR was called for unit testing
extern bool _errG;

__attribute__((__used__))
void sEventF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
__attribute__((__used__))
void sEventV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args);
__attribute__((__used__))
void sEvent(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sErrorF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
__attribute__((__used__))
void sErrorV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args);
__attribute__((__used__))
void sError(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sWarningF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
__attribute__((__used__))
void sWarningV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args);
__attribute__((__used__))
void sWarning(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

//__attribute__((__used__))
//void sInfoF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__ ((format (printf, 3, 4)));
__attribute__((__used__))
void sChanneledInfoF(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__ ((format (printf, 4, 5)));

//__attribute__((__used__))
//void sInfoV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args);
__attribute__((__used__))
void sChanneledInfoV(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args);
//__attribute__((__used__))
//void sInfo(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);
__attribute__((__used__))
void sChanneledInfo(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sChanneledDebugF(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__ ((format (printf, 4, 5)));
__attribute__((__used__))
void sChanneledDebugV(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args);
__attribute__((__used__))
void sChanneledDebug(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

void sSetGlobal(const char* key, const char* value);

} // namespace Util
} // namespace Anki

#define DPHYS "$phys"
#define DDATA "$data"
#define DGROUP "$group"
#define DGAME "$game"

#define DEFAULT_CHANNEL_NAME "Unnamed"

// send BI event
#define PRINT_NAMED_EVENT(name, format, ...) do{::Anki::Util::sEventF(name, {}, format, ##__VA_ARGS__);}while(0)

// Logging with names.
#define PRINT_NAMED_ERROR(name, format, ...) do{::Anki::Util::sErrorF(name, {}, format, ##__VA_ARGS__); ::Anki::Util::_errG=true; }while(0)
#define PRINT_NAMED_WARNING(name, format, ...) do{::Anki::Util::sWarningF(name, {}, format, ##__VA_ARGS__);}while(0)
#define PRINT_NAMED_INFO(name, format, ...) do{::Anki::Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, name, {}, format, ##__VA_ARGS__);}while(0)
#define PRINT_NAMED_DEBUG(name, format, ...) do{::Anki::Util::sChanneledDebugF(DEFAULT_CHANNEL_NAME, name, {}, format, ##__VA_ARGS__);}while(0)

// Logging with channels.
#define PRINT_CH_INFO(channel, name, format, ...) do{::Anki::Util::sChanneledInfoF(channel, name, {}, format, ##__VA_ARGS__);}while(0)
#define PRINT_CH_DEBUG(channel, name, format, ...) do{::Anki::Util::sChanneledDebugF(channel, name, {}, format, ##__VA_ARGS__);}while(0)

// Streams
#define PRINT_STREAM_ERROR(eventName, args) do{         \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sError(eventName, {}, ss.str().c_str()); \
    }while(0)

#define PRINT_STREAM_WARNING(eventName, args) do{       \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sWarning(eventName, {}, ss.str().c_str()); \
    }while(0)

#define PRINT_STREAM_INFO(eventName, args) do{          \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sChanneledInfo(DEFAULT_CHANNEL_NAME, eventName, {}, ss.str().c_str()); \
    }while(0)

#define PRINT_STREAM_DEBUG(eventName, args) do{         \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sChanneledDebug(DEFAULT_CHANNEL_NAME, eventName, {}, ss.str().c_str()); \
    }while(0)

// Auto streams
#define PRINT_AUTOSTREAM_DEBUG(args) do{ \
    char eventNameBuf[256]; GENERATE_EVENT_NAME(eventNameBuf, sizeof(eventNameBuf)); \
    PRINT_STREAM_DEBUG(eventNameBuf, args); }while(0)


#define SHORT_FILE ( strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__ )
#define GENERATE_EVENT_NAME(nameBuf, nameBufLen) { snprintf(nameBuf, nameBufLen, "%s.%s.%d", (SHORT_FILE), __FUNCTION__, __LINE__); }


// Anki assert definition
#if (!defined(NDEBUG)) && !(defined(UNIT_TEST))
#define DEBUG_ABORT __builtin_trap()
#else
#define DEBUG_ABORT ((void)0)
#endif

#define ASSERT_NAMED(exp, name) do{                                 \
          if(!(exp)) {                                              \
            PRINT_NAMED_ERROR(name, "Assertion Failed: %s", #exp);  \
            Anki::Util::sDumpCallstack("AssertCallstack");          \
            DEBUG_ABORT;                                            \
          }                                                         \
        }while(0)

#define ASSERT_NAMED_EVENT(exp, name, format, ...) do{                                \
          if(!(exp)) {                                                                \
            PRINT_NAMED_ERROR(name, "ASSERT ( %s ): " format, #exp, ##__VA_ARGS__);   \
            Anki::Util::sDumpCallstack("AssertCallstack");                            \
            DEBUG_ABORT;                                                              \
          }                                                                           \
        }while(0)

#endif // __Util_Logging_Logging_H_

