//
//  BLELog.h
//  BLEManager
//
//  Created by Mark Pauley on 5/21/13.
//  Copyright (c) 2013 Mark Pauley. All rights reserved.
//

#ifndef BLEManager_BLELog_h
#define BLEManager_BLELog_h

#ifndef USE_DAS
#define USE_DAS 0
#endif

#if (USE_DAS == 1)
#include <DAS/DAS.h>

//BLELogDebug
#define BLELogDebug DASDebug
//BLELogInfo
#define BLELogInfo  DASInfo
//BLELogEvent
#define BLELogEvent DASEvent
//BLELogWarn
#define BLELogWarn  DASWarn
//BLELogError
#define BLELogError DASError

#define BLEAssert   DASAssert

#else
// NO DAS
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

#endif // end NO DAS

#endif