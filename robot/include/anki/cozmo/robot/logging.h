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

// Which errors will be checked and reported?
#define ANKI_DEBUG_MINIMAL 0 // Only check and output issues with explicit unit tests
#define ANKI_DEBUG_ERRORS 10 // Check and output AnkiErrors and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS 20 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests
#define ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS 30 // Check and output AnkiErrors, AnkiWarns, AnkiAsserts, and explicit unit tests
#define ANKI_DEBUG_ALL 40 // Check and output AnkiErrors, AnkiWarns, and explicit unit tests, plus run any additional extensive tests

#ifndef ANKI_DEBUG_LEVEL
#define ANKI_DEBUG_LEVEL ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
#endif
#ifndef ANKI_DEBUG_EVENTS
#define ANKI_DEBUG_EVENTS 1
#endif
#ifndef ANKI_DEBUG_INFO
#define ANKI_DEBUG_INFO 1
#endif



#include "clad/types/robotLogging.h"

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
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_EVENT, nameId, fmtId, nargs, ##__VA_ARGS__); }
#else
      #define AnkiEvent(...)
#endif

#if ANKI_DEBUG_INFO
      #define AnkiInfo(nameId, nameString, fmtId, fmtString, nargs, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_INFO, nameId, fmtId, nargs, ##__VA_ARGS__); }
#else
      #define AnkiInfo(...)
#endif

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_MINIMAL
      #define AnkiDebug(nameId, nameString, fmtId, fmtString, nargs, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_DEBUG, nameId, fmtId, nargs, ##__VA_ARGS__); }
      
      #define AnkiDebugPeriodic(num_calls_between_prints, nameId, nameString, fmtId, fmtString, nargs, ...) \
      {   static u32 cnt = num_calls_between_prints; \
          if (cnt++ >= num_calls_between_prints) { \
            Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_DEBUG, nameId, fmtId, nargs, ##__VA_ARGS__); \
            cnt = 0; \
          } \
      }
#else
      #define AnkiDebug(...)
      #define AnkiDebugPeriodic(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
      #define AnkiError(nameId, nameString, fmtId, fmtString, nargs, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, ##__VA_ARGS__); }

      #define AnkiConditionalError(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, ##__VA_ARGS__); \
        }

      #define AnkiConditionalErrorAndReturn(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if(!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, nameId, fmtId, nargs, ##__VA_ARGS__); \
          return returnValue; \
        }
#else
      #define AnkiError(...)
      #define AnkiConditionalError (...)
      #define AnkiConditionalErrorAndReturn (...)
      #define AnkiConditionalErrorAndReturnValue(...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
      #define AnkiWarn(nameId, nameString, fmtId, fmtString, nargs, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, ##__VA_ARGS__); }

      #define AnkiConditionalWarn(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, ##__VA_ARGS__); \
        }

      #define AnkiConditionalWarnAndReturn(expression, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, nameId, nameString, fmtId, fmtString, nargs, ...) \
        if(!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, nameId, fmtId, nargs, ##__VA_ARGS__); \
          return returnValue;\
        }
#else
      #define AnkiWarn(...)
      #define AnkiConditionalWarn(...)
      #define AnkiConditionalWarnAndReturn(...)
      #define AnkiConditionalWarnAndReturnValue(...)
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
