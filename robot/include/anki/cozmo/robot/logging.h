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
#include "clad/types/robotLogging.h"
#include <stdio.h>
#include <stdlib.h>

#define console_printf(...) printf(__VA_ARGS__)

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
		
#if ANKI_DEBUG_EVENTS
      #define AnkiEvent(nameString, fmtString, ...) \
      { \
        console_printf("[Event] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
      }
#else
      #define AnkiEvent(...)
#endif

#if ANKI_DEBUG_INFO
      #define AnkiInfo(nameString, fmtString, ...) \
      { \
        console_printf("[Info] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
      }
#else
      #define AnkiInfo(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ALL
      #define AnkiDebug(nameString, fmtString, ...) \
      { \
        console_printf("[Debug] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
      }
      
      #define AnkiDebugPeriodic(num_calls_between_prints, nameString, fmtString, ...) \
      {   static u16 cnt = num_calls_between_prints; \
          if (++cnt > num_calls_between_prints) { \
            console_printf("[DebugP] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
            cnt = 0; \
          } \
      }
#else
      #define AnkiDebug(...)
      #define AnkiDebugPeriodic(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
      #define AnkiError(nameString, fmtString, ...) { \
        console_printf("[Error] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
      }

      #define AnkiConditionalError(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          console_printf("[Error] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
        }

      #define AnkiConditionalErrorAndReturn(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          console_printf("[Error] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, nameString, fmtString, ...) \
        if(!(expression)) { \
          console_printf("[Error] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
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
        console_printf("[Warn] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
      }

      #define AnkiConditionalWarn(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          console_printf("[Warn] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
        }

      #define AnkiConditionalWarnAndReturn(expression, nameString, fmtString, ...) \
        if (!(expression)) { \
          console_printf("[Warn] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, nameString, fmtString, ...) \
        if(!(expression)) { \
          console_printf("[Warn] " nameString ": " fmtString "\r\n", ##__VA_ARGS__); \
          return returnValue;\
        }
#else
      #define AnkiWarn(...)
      #define AnkiConditionalWarn(...)
      #define AnkiConditionalWarnAndReturn(expression, ...) if (!(expression)) return
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, ...) if (!(expression)) return returnValue
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
				
      // Anki assert sends assesrt CLAD message and then halts main exec
      #define STRINGIZE(x) #x
      #define AnkiAssert(expression, fmtString, ...) \
        if (!(expression)) { \
          console_printf("[Assert] " fmtString ": \"" #expression "\" failed at line " STRINGIZE(__LINE__) " in file " __FILE__ "\r\n", ##__VA_ARGS__); \
          exit(-1); \
        }
#else
      #define AnkiAssert(...)
#endif

    }
  }
}

#endif
