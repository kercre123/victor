/**
 * File: util/logging/DAS.h
 *
 * Description: DAS extensions for Util::Logging macros
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __util_logging_DAS_h
#define __util_logging_DAS_h

namespace Anki {
namespace Util {
namespace DAS {

//
// DAS v2 event fields
// https://ankiinc.atlassian.net/wiki/spaces/SAI/pages/221151429/DAS+Event+Fields+for+Victor+Robot
//
constexpr const char * SOURCE = "$source";
constexpr const char * EVENT = "$event";
constexpr const char * TS = "$ts";
constexpr const char * SEQ = "$seq";
constexpr const char * LEVEL = "$level";
constexpr const char * PROFILE = "$profile";
constexpr const char * ROBOT = "$robot";
constexpr const char * ROBOT_VERSION = "$robot_version";
constexpr const char * FEATURE_TYPE = "$feature_type";
constexpr const char * FEATURE_RUN = "$feature_run";
constexpr const char * STR1 = "$s1";
constexpr const char * STR2 = "$s2";
constexpr const char * STR3 = "$s3";
constexpr const char * STR4 = "$s4";
constexpr const char * INT1 = "$i1";
constexpr const char * INT2 = "$i2";
constexpr const char * INT3 = "$i3";
constexpr const char * INT4 = "$i4";

} // end namespace DAS
} // end namespace Util
} // end namespace Anki

#endif // __util_logging_DAS_h
