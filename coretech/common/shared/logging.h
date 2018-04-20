/**
* File: coretech/common/shared/logging.h
*
* Description:
*   Log macros for use within coretech
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef ANKICORETECH_COMMON_SHARED_LOGGING_H_
#define ANKICORETECH_COMMON_SHARED_LOGGING_H_

//
// Logging functions that provide a thin wrapper around Anki::Util macros, without bringing Anki::Util macros
// into the CoreTech namespace.
//

// Define coretech logging levels
#define CORETECH_LOGLEVEL_NONE    0
#define CORETECH_LOGLEVEL_ERROR   1
#define CORETECH_LOGLEVEL_WARNING 2
#define CORETECH_LOGLEVEL_INFO    3
#define CORETECH_LOGLEVEL_DEBUG   4

// Which level are we using for this configuration?
#if !defined(CORETECH_LOGLEVEL)
  #if defined(CORETECH_ROBOT)
    #define CORETECH_LOGLEVEL CORETECH_LOGLEVEL_INFO
  #elif defined(CORETECH_ENGINE)
    #define CORETECH_LOGLEVEL CORETECH_LOGLEVEL_DEBUG
  #else
    #error "You must define CORETECH_ROBOT or CORETECH_ENGINE"
  #endif
#endif

namespace Anki {
namespace CoreTech {

#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_ERROR)
// Log a named error. Name and format must not be null.
void LogError(const char * name, const char * format, ...) __attribute__((format(printf,2,3)));
#endif

#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_WARNING)
// Log a named warning. Name and format must not be null.
void LogWarning(const char * name, const char * format, ...) __attribute__((format(printf,2,3)));
#endif

#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_INFO)
// Log a named info message. Channel, name, and format must not be null.
void LogInfo(const char * channel, const char * name, const char * format, ...) __attribute__((format(printf,3,4)));
#endif

#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_DEBUG)
// Log a named debug message. Channel, name, and format must not be null.
void LogDebug(const char * channel, const char * name, const char * format, ...) __attribute__((format(printf,3,4)));
#endif

} // end namespace CoreTech
} // end namespace Anki

//
// Declare log macros for use outside of namespace.
// Channel, name, and format must not be null.
//
#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_ERROR)
#define CORETECH_LOG_ERROR(name, format, ...)          Anki::CoreTech::LogError(name, format, ##__VA_ARGS__)
#else
#define CORETECH_LOG_ERROR(name, format, ...)          {}
#endif

#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_WARNING)
#define CORETECH_LOG_WARNING(name, format, ...)        Anki::CoreTech::LogWarning(name, format, ##__VA_ARGS__)
#else
#define CORETECH_LOG_WARNING(name, format, ...)        {}
#endif

#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_INFO)
#define CORETECH_LOG_INFO(channel, name, format, ...)  Anki::CoreTech::LogInfo(channel, name, format, ##__VA_ARGS__)
#else
#define CORETECH_LOG_INFO(channel, name, format, ...)  {}
#endif

#if (CORETECH_LOGLEVEL >= CORETECH_LOGLEVEL_DEBUG)
#define CORETECH_LOG_DEBUG(channel, name, format, ...) Anki::CoreTech::LogDebug(channel, name, format, ##__VA_ARGS__)
#else
#define CORETECH_LOG_DEBUG(channel, name, format, ...) {}
#endif

#endif /* ANKICORETECH_COMMON_SHARED_LOGGING_H_ */
