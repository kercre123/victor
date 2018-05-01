/** Embedded logging facilities for Cozmo
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
#include <stdio.h>
#include <stdlib.h>

#if defined(ANDROID)

#include <android/log.h>
#define console_printf(fmt, ...) __android_log_print(ANDROID_LOG_INFO, "vic-robot", fmt, ##__VA_ARGS__)
#define log_assert(line, file, expr, fmt, ...) __android_log_assert(expr, "vic-robot", fmt ": failed at line " line " in file " file, ##__VA_ARGS__)
#define log_error(name, fmt, ...) __android_log_print(ANDROID_LOG_ERROR, "vic-robot", name ": " fmt, ##__VA_ARGS__)
#define log_warn(name, fmt, ...) __android_log_print(ANDROID_LOG_WARN, "vic-robot", name ": " fmt, ##__VA_ARGS__)
#define log_info(name, fmt, ...) __android_log_print(ANDROID_LOG_INFO, "vic-robot", name ": " fmt, ##__VA_ARGS__)
#define log_debug(name, fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, "vic-robot", name ": " fmt, ##__VA_ARGS__)

#else

#define console_printf(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define log_assert(line, file, expr, fmt, ...) console_printf("[Assert] " fmt ": \"" expr "\" failed at line " line " in file " file "\r\n", ##__VA_ARGS__)
#define log_error(name, fmt, ...) console_printf("[Error] " name ": " fmt "\r\n", ##__VA_ARGS__)
#define log_warn(name, fmt, ...) console_printf("[Warn] " name ": " fmt "\r\n", ##__VA_ARGS__)
#define log_info(name, fmt, ...) console_printf("[Info] " name ": " fmt "\r\n", ##__VA_ARGS__)
#define log_debug(name, fmt, ...) console_printf("[Debug] " name ": " fmt "\r\n", ##__VA_ARGS__)

#endif

// Keil doesn't seem to reliably error on these not being defined below so trigger explicitly.
#ifndef ANKI_DEBUG_INFO
#error ANKI_DEBUG_INFO not defined
#endif
#ifndef ANKI_DEBUG_LEVEL
#error ANKI_DEBUG_LEVEL not defined
#endif

namespace Anki {
  namespace Cozmo {
    namespace RobotInterface {

#if ANKI_DEBUG_INFO
      #define AnkiInfo(nameString, fmtString, ...) \
      { \
        log_info(nameString, fmtString, ##__VA_ARGS__); \
      }
#else
      #define AnkiInfo(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
      #define AnkiDebug(nameString, fmtString, ...) \
      { \
        log_debug(nameString, fmtString, ##__VA_ARGS__); \
      }

      #define AnkiDebugPeriodic(num_calls_between_prints, nameString, fmtString, ...) \
      {   static u16 cnt = num_calls_between_prints; \
          if (++cnt > num_calls_between_prints) { \
            log_debug(nameString, fmtString, ##__VA_ARGS__); \
            cnt = 0; \
          } \
      }
#else
      #define AnkiDebug(...)
      #define AnkiDebugPeriodic(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
      #define AnkiError(nameString, fmtString, ...) { \
        log_error(nameString, fmtString, ##__VA_ARGS__); \
      }

      #define AnkiConditionalError(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          log_error(nameString, fmtString, ##__VA_ARGS__); \
        }

      #define AnkiConditionalErrorAndReturn(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          log_error(nameString, fmtString, ##__VA_ARGS__); \
          return; \
        }

      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, nameString, fmtString, ...) \
        if(!(expression)) { \
          log_error(nameString, fmtString, ##__VA_ARGS__); \
          return returnValue; \
        }
#else
      #define AnkiError(...)
      #define AnkiConditionalError (...)
      #define AnkiConditionalErrorAndReturn (expression, ...) if (!(expression)) return
      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, ...) if (!(expression)) return returnValue
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
      #define AnkiWarn(nameString, fmtString, ...) { \
        log_warn(nameString, fmtString, ##__VA_ARGS__); \
      }

      #define AnkiConditionalWarn(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          log_warn(nameString, fmtString, ##__VA_ARGS__); \
        }

      #define AnkiConditionalWarnAndReturn(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          log_warn(nameString, fmtString, ##__VA_ARGS__); \
          return; \
        }

      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, nameString, fmtString, ...) \
        if(!(expression)) { \
          log_warn(nameString, fmtString, ##__VA_ARGS__); \
          return returnValue;\
        }
#else
      #define AnkiWarn(...)
      #define AnkiConditionalWarn(...)
      #define AnkiConditionalWarnAndReturn(expression, ...) if (!(expression)) return
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, ...) if (!(expression)) return returnValue
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
      // Extra level of expansion required for proper stringification of __LINE__
      #define _STRINGIZE(x) #x
      #define STRINGIZE(x) _STRINGIZE(x)
      // Anki assert sends assert CLAD message and then halts main exec
      #define AnkiAssert(expression, fmtString, ...) \
        if (!(expression)) { \
          log_assert(STRINGIZE(__LINE__), __FILE__, #expression, fmtString, ##__VA_ARGS__); \
          exit(-1); \
        }
#else
      #define AnkiAssert(...)
#endif

    }
  }
}

#endif
