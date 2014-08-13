/**
 * File: logging
 *
 * Author: damjan
 * Created: 4/3/2014
 *
 * Description: print macros
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef UTIL_LOGGING_LOGGING_H_
#define UTIL_LOGGING_LOGGING_H_

#define FAKE_DAS

#ifndef FAKE_DAS
#include <DAS/DAS.h>
#else
#include "anki/common/basestation/utils/logging/DAS/DAS.h"
#endif


namespace AnkiUtil {

// global error flag so we can check if PRINT_ERROR was called for unit testing
extern bool _errG;

// Print error message to stderr. First flushes stdout to make sure print order
// is correct.
// #define PRINT_ERROR(format, ...) do{ _errG=true; DASErrorAuto(format, ##__VA_ARGS__); }while(0)
// #define PRINT_WARNING(format, ...) do{DASWarnAuto(format, ##__VA_ARGS__);}while(0)
#define PRINT_INFO(format, ...) do{DASInfoAuto(format, ##__VA_ARGS__);}while(0)
#define PRINT_DEBUG(format, ...) do{DASDebugAuto(format, ##__VA_ARGS__);}while(0)
#define PRINT_DAS_BY_TYPE(type, format, ...) do{ switch(type) { case DAS_CATEGORY_DEBUG: PRINT_DEBUG(format,##__VA_ARGS__); break; \
                                                                case DAS_CATEGORY_INFO: PRINT_INFO(format,##__VA_ARGS__); break; \
                                                                case DAS_CATEGORY_WARNING: PRINT_WARNING(format,##__VA_ARGS__); break; \
                                                                case DAS_CATEGORY_ERROR: PRINT_ERROR(format,##__VA_ARGS__); break; \
                                                                default: break; }}while(0)

// Logging with names. This is preferable to the PRINT_* messages above
#define PRINT_NAMED_ERROR(name, format, ...) do{ ::AnkiUtil::_errG=true; DASError(name, format, ##__VA_ARGS__); }while(0)
#define PRINT_NAMED_WARNING(name, format, ...) do{DASWarn(name, format, ##__VA_ARGS__);}while(0)
#define PRINT_NAMED_INFO(name, format, ...) do{DASInfo(name, format, ##__VA_ARGS__);}while(0)
#define PRINT_NAMED_DEBUG(name, format, ...) do{DASDebug(name, format, ##__VA_ARGS__);}while(0)
  
  
} // namespace Utilß

#endif // UTIL_LOGGING_LOGGING_H_

