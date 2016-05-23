//
//  dasNSLog.m
//  DAS
//
//  Created by Stuart Eichert on 1/21/15.
//  Copyright (c) 2015 Anki, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#include "dasNSLog.h"

void DASNSLog(const char* message) {
    NSLog(@"%s", message);
}

void DASNSLogV(const char* fmt, ...) {
    char logBuf[4096];
    va_list argList;
    va_start(argList, fmt);
    vsprintf(logBuf, fmt, argList);
    DASNSLog(logBuf);
    va_end(argList);
}
