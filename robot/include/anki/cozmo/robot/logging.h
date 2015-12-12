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

/** Hideous macro recursion chain for counting the number of arguments passed to a variatic macro
 */
#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0         
// End macro abhorence

#include "clad/types/robotLogging.h"

namespace Anki {
  namespace Cozmo {
    namespace RobotInterface {
      /** Sends a log event to the base station.
       * @param level The event log level
       * @param name The event enum / name / format string
       * @param numArgs (int) The number of variable arguments
       * Remaining args are packed into array / format string
       */
      int SendLog(const LogLevel level, ...);
		
#if ANKI_DEBUG_EVENTS
      #define AnkiEvent(eventName, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_EVENT, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); }
#else
      #define AnkiEvent(eventName, ...)
#endif

#if ANKI_DEBUG_INFO
      #define AnkiInfo(eventName, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_INFO, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); }
#else
      #define AnkiInfo(eventName, ...)
#endif

#if ANKI_DEBUG_LEVEL > ANKI_DEBUG_MINIMAL
      #define AnkiDebug(eventName, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_DEBUG, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); }
#else
      #define AnkiDebug(eventName, ...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS
      #define AnkiError(eventName, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); }

      #define AnkiConditionalError(expression, eventName, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); \
        }

      #define AnkiConditionalErrorAndReturn(expression, eventName, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, eventName, ...) \
        if(!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ERROR, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); \
          return returnValue; \
        }
#else
      #define AnkiError(eventName, ...)
      #define AnkiConditionalError (espression, eventName, ...)
      #define AnkiConditionalErrorAndReturn (espression, eventName, ...)
      #define AnkiConditionalErrorAndReturnValue(expression, returnValue, eventName, ...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS
      #define AnkiWarn(eventName, ...) \
      { Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); }

      #define AnkiConditionalWarn(espression, eventName, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); \
        }

      #define AnkiConditionalWarnAndReturn(expression, eventName, ...) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); \
          return; \
        }
      
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, eventName, ...) \
        if(!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_WARN, eventName, PP_NARG(__VA_ARGS__), ##__VA_ARGS__); \
          return returnValue;\
        }
#else
      #define AnkiWarn(eventName, ...)
      #define AnkiConditionalWarn(espression, eventName, ...)
      #define AnkiConditionalWarnAndReturn(espression, eventName, ...)
      #define AnkiConditionalWarnAndReturnValue(expression, returnValue, eventName, ...)
#endif

#if ANKI_DEBUG_LEVEL >= ANKI_DEBUG_ERRORS_AND_WARNS_AND_ASSERTS
      #define AnkiAssert(expression, eventName) \
        if (!(expression)) { \
          Anki::Cozmo::RobotInterface::SendLog(Anki::Cozmo::RobotInterface::ANKI_LOG_LEVEL_ASSERT, eventName); \
          assert(false); \
        }
#else
      #define AnkiAssert(expression, eventName)
#endif

    }
  }
}


#endif
