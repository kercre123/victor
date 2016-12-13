/** Emedded logging facilities for Cozmo
 * @author Daniel Casner <daniel@anki.com>
 * @copyright Anki Inc. 2015
 * For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
 * File created Dec 2015
 *
 * API is intended to be similar to previous errorHandling and DAS libraries but with low overhead appropriate to
 * Cozmo's real time and network processors.
 *
 * Implementations of SendLog will vary for the different processors:
 * On the WiFi processor, SendLog will accept a level and a format string and variable arguments for payload
 * On the Real time processor, SendLog will accept a level and a RtipTrace enum from robotLogging.clad along with the
 * variable arguments for payload.
 */

#ifndef __ANKI_COZMO_ROBOT_LOGGING_H_
#define __ANKI_COZMO_ROBOT_LOGGING_H_

#include "anki/cozmo/robot/buildTypes.h"

template<typename T>
inline int trace_cast(const T arg)
{
  return (int)arg;
}

// No floats on the espressif
#ifndef TARGET_ESPRESSIF
template<> inline int trace_cast(const float arg)
{
  return *((int*)&arg);
}

template<> inline int trace_cast(const double arg)
{
  const float fltArg = (float)arg;
  return *((int*)&fltArg);
}
#endif

// Macro ball based loosely on http://stackoverflow.com/questions/6707148/foreach-macro-on-macros-arguments#6707531
// Paste macro is two layers to force extra eval
#define Paste(a,b) a ## b
#define XPASTE(a,b) Paste(a, b)
#define  CAST0()  0
#define  CAST1(a) trace_cast(a)
#define  CAST2(a, b) CAST1(a),  CAST1(b)
#define  CAST3(a, b, c) CAST1(a),  CAST2(b, c)
#define  CAST4(a, b, c, d) CAST1(a),  CAST3(b, c, d)
#define  CAST5(a, b, c, d, e) CAST1(a),  CAST4(b, c, d, e)
#define  CAST6(a, b, c, d, e, f) CAST1(a),  CAST5(b, c, d, e, f)
#define  CAST7(a, b, c, d, e, f, g) CAST1(a),  CAST6(b, c, d, e, f, g)
#define  CAST8(a, b, c, d, e, f, g, h) CAST1(a),  CAST7(b, c, d, e, f, g, h)
#define  CAST9(a, b, c, d, e, f, g, h, i) CAST1(a),  CAST8(b, c, d, e, f, g, h, i)
#define CAST10(a, b, c, d, e, f, g, h, i, j) CAST1(a),  CAST9(b, c, d, e, f, g, h, i, j)
#define CAST11(a, b, c, d, e, f, g, h, i, j, k) CAST1(a), CAST10(b, c, d, e, f, g, h, i, j, k)
#define CAST12(a, b, c, d, e, f, g, h, i, j, k, l) CAST1(a), CAST11(b, c, d, e, f, g, h, i, j, k, l)
#define CASTn(M, ...) M(__VA_ARGS__)
#define CASTx(nargs, ...) CASTn(XPASTE(CAST, nargs), __VA_ARGS__)

#include "clad/types/robotLogging.h"

#ifdef ESPRESSIF_CONSOLE_LOGGING
#define console_printf(...) os_printf(__VA_ARGS__)
#else
#define console_printf(...)
#endif

// Keil doesn't seem to reliably error on these not being defined below so trigger explictily.
#ifndef ANKI_DEBUG_EVENTS
#error ANKI_DEBUG_EVENTS not defined
#endif
#ifndef ANKI_DEBUG_INFO
#error ANKI_DEBUG_EVENTS not defined
#endif
#ifndef ANKI_DEBUG_LEVEL
#error ANKI_DEBUG_LEVEL not defined
#endif

namespace Anki {
  namespace Cozmo {
    namespace RobotInterface {
      /** Sends a log event to the base station.
       * @param level The event log level
       * @param name The name of the event / warning / error / etc.
       * @param formatId The ID of the format string for this event
       * @param numArgs The number of var args for the format
       * Remaining args are packed into array and used by format string on base station
       */
      int SendLog(const LogLevel level, const uint16_t name, const uint16_t formatId, const uint8_t numArgs, ...);
		
#if ANKI_DEBUG_EVENTS
      #define AnkiEvent(nameId, nameString, fmtId, fmtString, nargs, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_EVENT, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); }
#else
      #define AnkiEvent(...)
#endif

#if ANKI_DEBUG_INFO
      #define AnkiInfo(nameId, nameString, fmtId, fmtString, nargs, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_INFO, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); }
#else
      #define AnkiInfo(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
      #define AnkiDebug(nameId, nameString, fmtId, fmtString, nargs, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_DEBUG, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); }
      
      #define AnkiDebugPeriodic(num_calls_between_prints, nameId, nameString, fmtId, fmtString, nargs, ...) \
      {   static u16 cnt = num_calls_between_prints; \
          if (++cnt > num_calls_between_prints) { \
            Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_DEBUG, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
            cnt = 0; \
          } \
      }
#else
      #define AnkiDebug(...)
      #define AnkiDebugPeriodic(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
      #define AnkiError(nameId, nameString, fmtId, fmtString, nargs, ...) { \
        Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
        console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
      }

      #define AnkiConditionalError(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
          console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
        }

      #define AnkiConditionalErrorAndReturn(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
          console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if(!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
          console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
          return returnValue; \
        }
#else
      #define AnkiError(...)
      #define AnkiConditionalError (...)
      #define AnkiConditionalErrorAndReturn (expression, ...) if (!(expression)) return
      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, ...) if (!(expression)) return returnValue
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
      #define AnkiWarn(nameId, nameString, fmtId, fmtString, nargs, ...) { \
        Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
        console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
      }

      #define AnkiConditionalWarn(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
          console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
        }

      #define AnkiConditionalWarnAndReturn(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
          console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if(!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, CASTx(nargs, __VA_ARGS__)); \
          console_printf(nameString fmtString "\r\n", ##__VA_ARGS__); \
          return returnValue;\
        }
#else
      #define AnkiWarn(...)
      #define AnkiConditionalWarn(...)
      #define AnkiConditionalWarnAndReturn(expression, ...) if (!(expression)) return
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, ...) if (!(expression)) return returnValue
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS

#if defined(TARGET_ESPRESSIF)
	#define ANKI_ASSERT_SHOW Face::FacePrintf("ASSERT in " __FILE__ ", line %d", __LINE__)
#elif defined(TARGET_K02)
 #define ANKI_ASSERT_SHOW for (int i=0; i<5; ++i) Anki::Cozmo::HAL::SetLED(i, 0x7c00)
#else
 #define ANKI_ASSERT_SHOW
#endif
				
      // Anki assert sends assesrt CLAD message and then halts main exec
      #define AnkiAssert(expression, fmtId) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ASSERT, 0, fmtId, 1, __LINE__); \
          ANKI_ASSERT_SHOW; \
          while(true); \
        }
#else
      #define AnkiAssert(...)
#endif

    }
  }
}

#if defined(TARGET_ESPRESSIF)
	extern "C" void FacePrintf(const char *format, ...); // Forward declaration instead of include because we don't want that include everywhere this is.
#elif defined(TARGET_K02)
 // Forward declaration instead of include because we don't want that include everywhere this is.
 namespace Anki { namespace Cozmo { namespace HAL {
	void SetLED(uint8_t led_id, uint16_t color);
 }}}
#endif

#endif
