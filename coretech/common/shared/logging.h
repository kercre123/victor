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

namespace Anki {
namespace CoreTech {

// Log a named error. Name and format must not be null.
void LogError(const char * name, const char * format, ...) __attribute__((format(printf,2,3)));

// Log a named warning. Name and format must not be null.
void LogWarning(const char * name, const char * format, ...) __attribute__((format(printf,2,3)));

// Log a named info message. Channel, name, and format must not be null.
void LogInfo(const char * channel, const char * name, const char * format, ...) __attribute__((format(printf,3,4)));

// Log a named debug message. Channel, name, and format must not be null.
void LogDebug(const char * channel, const char * name, const char * format, ...) __attribute__((format(printf,3,4)));

} // end namespace CoreTech
} // end namespace Anki

//
// Declare log macros for use outside of namespace.
// Channel, name, and format must not be null.
//
#define CORETECH_LOG_ERROR(name, format, ...)          Anki::CoreTech::LogError(name, format, ##__VA_ARGS__)
#define CORETECH_LOG_WARNING(name, format, ...)        Anki::CoreTech::LogWarning(name, format, ##__VA_ARGS__)
#define CORETECH_LOG_INFO(channel, name, format, ...)  Anki::CoreTech::LogInfo(channel, name, format, ##__VA_ARGS__)
#define CORETECH_LOG_DEBUG(channel, name, format, ...) Anki::CoreTech::LogDebug(channel, name, format, ##__VA_ARGS__)

#endif /* ANKICORETECH_COMMON_SHARED_LOGGING_H_ */
