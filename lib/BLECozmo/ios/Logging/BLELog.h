//
//  BLELog.h
//  BLEManager
//
//  Created by Mark Pauley on 5/21/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#ifndef BLEManager_BLELog_h
#define BLEManager_BLELog_h

#define BLECOZMO_FORCE_PRINT 1
#define BLECOZMO_STANDALONE 1

#if !BLECOZMO_STANDALONE
#include "util/logging/logging.h"

#if BLECOZMO_FORCE_PRINT
//BLELogDebug
#define BLELogDebug PRINT_NAMED_DEBUG
//BLELogInfo
#define BLELogInfo  PRINT_NAMED_INFO
//BLELogEvent
#define BLELogEvent PRINT_NAMED_EVENT
//BLELogWarn
#define BLELogWarn  PRINT_NAMED_WARNING
//BLELogError
#define BLELogError PRINT_NAMED_ERROR

#define xstr(s) str(s)
#define str(s) #s
#define BLEAssert(_test)  ASSERT_NAMED((_test), "BLEAssert." __FILE__ ":" xstr(__LINE__) )

#else
// NO Logging

#define BLENoLog(who, what, ...)

#define BLELogDebug BLENoLog
#define BLELogInfo  BLENoLog
#define BLELogEvent BLENoLog
#define BLELogWarn  BLENoLog
#define BLELogError BLENoLog
#define BLEAssert(_test)

#endif // end debug level or force print

#else // IS Standalone

#define BLELogDebug BLELog
#define BLELogInfo  BLELog
#define BLELogEvent BLELog
#define BLELogWarn  BLELog
#define BLELogError BLELog

// weak sauce!
#ifndef UNIT_TEST
#define BLELog(who, what, ...) do{printf("BLE(" who "): " what "\n", ##__VA_ARGS__);}while(0)
#else
#define BLELog(who, what, ...)
#endif

#define BLEAssert assert

#endif // end IS Standalone

#endif // BLEManager_BLELog_h