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
 *   If you however feel that we should stack them into one set of function that uses LogLevel as a param, think about
 *   need to translate Ank::Util::LogLevel to DasLogLevel and then to ios/android LogLevel.
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef __Util_Logging_Logging_H_
#define __Util_Logging_Logging_H_

#include "util/global/globalDefinitions.h"
#include "util/logging/eventKeys.h"
#include "util/logging/callstack.h"

#include <string>
#include <vector>

#ifndef ALLOW_DEBUG_LOGGING
#define ALLOW_DEBUG_LOGGING ANKI_DEVELOPER_CODE
#endif

namespace Anki {
namespace Util {

class ITickTimeProvider;
class ILoggerProvider;
class ChannelFilter;
class IEventProvider;

const uint8_t DASMaxSvalLength = 128;

std::string HexDump(const void *value, const size_t len, char delimiter);

extern ITickTimeProvider* gTickTimeProvider;
extern ILoggerProvider* gLoggerProvider;
extern ChannelFilter gChannelFilter;
extern IEventProvider* gEventProvider;

// Global error flag so we can check if PRINT_ERROR was called for unit testing
extern bool _errG;

// Global flag to control break-on-error behavior
extern bool _errBreakOnError;

/*! \defgroup dasmsg Das Messages
    \brief Shown when the cozmo anim update to slow to respond

  This will be show on the slow updates and as such should not be seen very often
*/
class DasMsg
{
public:
  virtual const char* eventName() = 0;
  void formatEventValue(const char* format, va_list args);
  std::string _eventValue;
};

#define DASMESSAGE(className, eventNameStr) /** \ingroup dasmsg */ \
                                            /** \brief eventNameStr */ \
                                            class className: public Anki::Util::DasMsg \
                                            { \
                                            public: \
                                              className(const char* format, ...) { va_list args; va_start(args, format); formatEventValue(format, args); va_end(args); } \
                                              virtual const char* eventName() { return eventNameStr; } \
                                            };

__attribute__((__used__))
void sEventF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__((format(printf,3,4)));

__attribute__((__used__))
void sEventV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args) __attribute__((format(printf,3,0)));

__attribute__((__used__))
void sEventD(DasMsg& dasMessage);

__attribute__((__used__))
void sEvent(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sErrorF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__((format(printf,3,4)));

__attribute__((__used__))
void sErrorV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args) __attribute__((format(printf,3,0)));

__attribute__((__used__))
void sError(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sWarningF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__((format(printf,3,4)));

__attribute__((__used__))
void sWarningV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args) __attribute__((format(printf,3,0)));

__attribute__((__used__))
void sWarning(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sInfoF(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__((format(printf,3,4)));

__attribute__((__used__))
void sChanneledInfoF(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__((format(printf,4,5)));

__attribute__((__used__))
void sInfoV(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args) __attribute__((format(printf,3,0)));

__attribute__((__used__))
void sChanneledInfoV(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args) __attribute__((format(printf,4,0)));

__attribute__((__used__))
void sInfo(const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sChanneledInfo(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

__attribute__((__used__))
void sChanneledDebugF(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, ...) __attribute__((format(printf,4,5)));

__attribute__((__used__))
void sChanneledDebugV(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* format, va_list args) __attribute__((format(printf,4,0)));

__attribute__((__used__))
void sChanneledDebug(const char* channelName, const char* eventName, const std::vector<std::pair<const char*, const char*>>& keyValues, const char* eventValue);

// Helper for use with ANKI_VERIFY macro. Always returns false.
__attribute__((__used__))
bool sVerifyFailedReturnFalse(const char* eventName, const char* format, ...) __attribute__((format(printf,2,3)));


void sSetGlobal(const char* key, const char* value);


//
// Anki::Util::sLogFlush()
// Perform synchronous flush of log data to underlying storage.
// This calls blocks until log data has been flushed.
//
void sLogFlush();

//
// Anki::Util::sDebugBreak()
// Break to debugger (if possible), then return to caller.
// If break to debugger is not supported, this function provides
// a convenient hook for developers to set a breakpoint by hand.
// This function is enabled for build configurations with ANKI_DEVELOPER_CODE=1.
// This function is a no-op for build configurations with ANKI_DEVELOPER_CODE=0.
//
void sDebugBreak();

// Anki::Util::sDebugBreakOnError()
// Calls sDebugBreak() in error situations if the configuration
// allows it. This is a separate function rather than a macro
// so that its behavior isn't affected by different configurations
// at different levels of the project (i.e. DriveEngine has a
// different setting than the OverDrive app or something like that)
//
void sDebugBreakOnError();

//
// Anki::Util::sAbort()
// Dump core (if possible) and terminate process.
// Does not flush buffers.
// Does not invoke exit handlers.
// Never returns to caller.
//
__attribute__((noreturn)) void sAbort();

} // namespace Util
} // namespace Anki

#define DEFAULT_CHANNEL_NAME "Unnamed"

//
// Logging with names.
//
#define PRINT_NAMED_ERROR(name, format, ...) do { \
  ::Anki::Util::sErrorF(name, {}, format, ##__VA_ARGS__); \
  ::Anki::Util::_errG=true; \
  if (::Anki::Util::_errBreakOnError) { \
    ::Anki::Util::sDebugBreakOnError(); \
  } \
} while(0)

#define PRINT_NAMED_WARNING(name, format, ...) do { \
  ::Anki::Util::sWarningF(name, {}, format, ##__VA_ARGS__); \
} while(0)

#define PRINT_NAMED_INFO(name, format, ...) do { \
  ::Anki::Util::sChanneledInfoF(DEFAULT_CHANNEL_NAME, name, {}, format, ##__VA_ARGS__); \
} while(0)

#if ALLOW_DEBUG_LOGGING
#define PRINT_NAMED_DEBUG(name, format, ...) do { \
  ::Anki::Util::sChanneledDebugF(DEFAULT_CHANNEL_NAME, name, {}, format, ##__VA_ARGS__); \
} while(0)
#else
#define PRINT_NAMED_DEBUG(name, format, ...)
#endif

//
// ANKI_VERIFY(expr, name, format, args...)
// Helper macro for simple error checks / assertions.
// Similar to DEV_ASSERT (below) but enabled in both debug and release (and shipping) builds.
//
// If the conditional expression (expr) is true, ANKI_VERIFY returns true.
// If the conditional expression (expr) is false, ANKI_VERIFY logs an error message, dumps the callstack,
// and returns false.
//
// The conditional expression (expr) will be evaluated only once. Arguments required to produce the formatted
// string for the log will only be evaluated if expr==false.
//
// Example 1:
// Use
//   if (ANKI_VERIFY(x == y, "VerifyXY", "%p != %p", x, y)) {
//     /* do stuff */
//   }
// in place of
//   if (x == y) {
//     /* do stuff */
//   } else {
//     PRINT_NAMED_ERROR("VerifyXY", "%p != p", x, y);
//     sDumpCallstack();
//   }
//
// Example 2:
// Use
//   if (!ANKI_VERIFY(x == y, "VerifyXY", "%p != %p", x, y)) {
//     return FAIL;
//   }
// in place of
//   if (x != y) {
//     PRINT_NAMED_ERROR("VerifyXY", "%p != %p", x, y);
//     sDumpCallstack();
//     return FAIL;
//   }
//
// Note that "&& false" is used to inform static analysis that the "verify failed" branch always returns false.
// This prevents analyzers from generating bogus warnings caused by impossible code paths.
//
#define ANKI_VERIFY(expr, name, format, ...) \
  (expr ? true : (::Anki::Util::sVerifyFailedReturnFalse(name, "VERIFY(%s): " format, #expr, ##__VA_ARGS__) && false))

//
// Logging with channels.
//
#define PRINT_CH_INFO(channel, name, format, ...) do { \
  ::Anki::Util::sChanneledInfoF(channel, name, {}, format, ##__VA_ARGS__); \
} while(0)

#define PRINT_CH_DEBUG(channel, name, format, ...) do { \
  ::Anki::Util::sChanneledDebugF(channel, name, {}, format, ##__VA_ARGS__); \
} while(0)

//
// Periodic logging with channels.
//

// Helper used by debug/info versions below
#define PRINT_PERIODIC_CH_HELPER(func, period, channel, name, format, ...) \
{ static u16 cnt = period;                                                 \
  if (++cnt >= period) {                                                   \
    ::Anki::Util::func(channel, name, {}, format, ##__VA_ARGS__);          \
    cnt = 0;                                                               \
  }                                                                        \
}

// Actually use these in your code (not the helper above)
#define PRINT_PERIODIC_CH_INFO(period, channel, name, format, ...) \
PRINT_PERIODIC_CH_HELPER(sChanneledInfoF, period, channel, name, format, ##__VA_ARGS__)

#define PRINT_PERIODIC_CH_DEBUG(period, channel, name, format, ...) \
PRINT_PERIODIC_CH_HELPER(sChanneledDebugF, period, channel, name, format, ##__VA_ARGS__)

// Streams
#define PRINT_STREAM_ERROR(eventName, args) do{         \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sError(eventName, {}, ss.str().c_str()); \
    } while(0)

#define PRINT_STREAM_WARNING(eventName, args) do{       \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sWarning(eventName, {}, ss.str().c_str()); \
    } while(0)

#define PRINT_STREAM_INFO(eventName, args) do{          \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sChanneledInfo(DEFAULT_CHANNEL_NAME, eventName, {}, ss.str().c_str()); \
    } while(0)

#if ALLOW_DEBUG_LOGGING
#define PRINT_STREAM_DEBUG(eventName, args) do{         \
      std::stringstream ss; ss<<args;                   \
      ::Anki::Util::sChanneledDebug(DEFAULT_CHANNEL_NAME, eventName, {}, ss.str().c_str()); \
    } while(0)
#else
#define PRINT_STREAM_DEBUG(eventName, args)
#endif

// Auto streams
#if ALLOW_DEBUG_LOGGING
#define PRINT_AUTOSTREAM_DEBUG(args) do{ \
    char eventNameBuf[256]; GENERATE_EVENT_NAME(eventNameBuf, sizeof(eventNameBuf)); \
    PRINT_STREAM_DEBUG(eventNameBuf, args); }while(0)
#else
#define PRINT_AUTOSTREAM_DEBUG(args)
#endif

#define SHORT_FILE ( strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__ )
#define GENERATE_EVENT_NAME(nameBuf, nameBufLen) { snprintf(nameBuf, nameBufLen, "%s.%s.%d", (SHORT_FILE), __FUNCTION__, __LINE__); }


// Anki assert definition
#if defined(NDEBUG) || defined(UNIT_TEST)
#define DEBUG_ABORT ((void)0)
#else
#define DEBUG_ABORT __builtin_trap()
#endif

#define ASSERT_NAMED(expr, name) do {                       \
  if (!(expr)) {                                            \
    PRINT_NAMED_ERROR(name, "Assertion Failed: %s", #expr); \
    Anki::Util::sDumpCallstack("AssertCallstack");          \
    Anki::Util::sLogFlush();                                \
    DEBUG_ABORT;                                            \
  }                                                         \
} while(0)

#define ASSERT_NAMED_AND_RETURN_FALSE_IF_FAIL(exp, name) do { \
  if(!(exp)) {                                              \
    PRINT_NAMED_ERROR(name, "Assertion Failed: %s", #exp);  \
    Anki::Util::sDumpCallstack("AssertCallstack");          \
    Anki::Util::sLogFlush();                                \
    DEBUG_ABORT;                                            \
    return false;                                           \
  }                                                         \
}while(0)


#define ASSERT_NAMED_EVENT(expr, name, format, ...) do {                      \
  if (!(expr)) {                                                              \
    PRINT_NAMED_ERROR(name, "ASSERT ( %s ): " format, #expr, ##__VA_ARGS__);  \
    Anki::Util::sDumpCallstack("AssertCallstack");                            \
    Anki::Util::sLogFlush();                                                  \
    DEBUG_ABORT;                                                              \
  }                                                                           \
} while(0)


#define ASSERT_NAMED_EVENT_AND_RETURN_FALSE_IF_FAIL(exp, name, format, ...) do { \
  if(!(exp)) {                                                                \
    PRINT_NAMED_ERROR(name, "ASSERT ( %s ): " format, #exp, ##__VA_ARGS__);   \
    Anki::Util::sDumpCallstack("AssertCallstack");                            \
    Anki::Util::sLogFlush();                                                  \
    DEBUG_ABORT;                                                              \
    return false;                                                             \
  }                                                                           \
}while(0)


//
// Developer assertions are compiled for debug builds ONLY.
// Developer assertions are discarded for release and shipping builds.
//
// Code blocks that are only used for developer assertions should be guarded with #if DEV_ASSERT_ENABLED.
// Variables that are only used for developer assertions should be guarded with DEV_ASSERT_ONLY.

#define DEV_ASSERT_ENABLED ANKI_DEVELOPER_CODE

#if DEV_ASSERT_ENABLED

#define DEV_ASSERT_MSG(expr, name, format, ...) do { \
  if (!(expr)) { \
    PRINT_NAMED_ERROR(name, "ASSERT(%s): " format, #expr, ##__VA_ARGS__); \
    Anki::Util::sDumpCallstack("ASSERT"); \
    Anki::Util::sLogFlush(); \
    Anki::Util::sAbort(); \
  } \
} while (0)

#define DEV_ASSERT_ONLY(expr) expr

#else

//
// Code within "if false" will be analyzed by compiler, so variables are counted as "used",
// but the entire block will be discarded by the optimizer because it can't be executed.
//
#define DEV_ASSERT_MSG(expr, name, format, ...) do { \
  if (false) { \
    if (!(expr)) { \
      PRINT_NAMED_ERROR(name, "ASSERT(%s): " format, #expr, ##__VA_ARGS__); \
    } \
  } \
} while (0)

#define DEV_ASSERT_ONLY(expr)

#endif

#define DEV_ASSERT(expr, name) DEV_ASSERT_MSG(expr, name, "Assertion failed")

//
// DAS events are structured messages for use with backend analytics.
// Event name and data fields are determined by the analytics team.
// Do NOT use LOG_EVENT to report random messages from your code!
//
#define DPHYS "$phys"
#define DDATA "$data"
#define DGROUP "$group"
#define DGAME "$game"
#define DCONNECTSESSION "$session_id"

// send BI event
#define LOG_EVENT(name, format, ...) do { \
  ::Anki::Util::sEventF(name, {}, format, ##__VA_ARGS__); \
} while(0)


//
// Compact version of PRINT_NAMED_ERROR & friends
// LOG_INFO and LOG_DEBUG assume you declared something like
//  #define LOG_CHANNEL "Name"
// near the top of your cpp file.
//
#define LOG_ERROR(name, fmt, ...)   PRINT_NAMED_ERROR(name, fmt, ##__VA_ARGS__)
#define LOG_WARNING(name, fmt, ...) PRINT_NAMED_WARNING(name, fmt, ##__VA_ARGS__)
#define LOG_INFO(name, fmt, ...)    PRINT_CH_INFO(LOG_CHANNEL, name, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(name, fmt, ...)   PRINT_CH_DEBUG(LOG_CHANNEL, name, fmt, ##__VA_ARGS__)

#define LOG_PERIODIC_INFO(period, name, fmt, ...) \
  PRINT_PERIODIC_CH_INFO(period, LOG_CHANNEL, name, fmt, ##__VA_ARGS__)
#define LOG_PERIODIC_DEBUG(period, name, fmt, ...) \
  PRINT_PERIODIC_CH_DEBUG(period, LOG_CHANNEL, name, fmt, ##__VA_ARGS__)

//
// Compact version of PRINT_CH_INFO & PRINT_CH_DEBUG macros.
// These macros can be used in header files or templates
// where "#define LOG_CHANNEL" is not appropriate.
//
#define LOG_CH_INFO(ch, name, fmt, ...)  PRINT_CH_INFO(ch, name, fmt, ##__VA_ARGS__)
#define LOG_CH_DEBUG(ch, name, fmt, ...) PRINT_CH_DEBUG(ch, name, fmt, ##__VA_ARGS__)

#endif // __Util_Logging_Logging_H_
