/**
 * File: dasLogMacros
 *
 * Author: seichert
 * Created: 01/15/2015
 *
 * Description: Macros for DAS implementation to log its own state
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __DasLogMacros_H__
#define __DasLogMacros_H__

#include "dasNSLog.h"

#define LOGD(fmt, ...) {DASNSLogV(fmt, __VA_ARGS__);}

#ifdef NDEBUG
#define LOGV(fmt, ...)
#else
#define LOGV(fmt, ...) {DASNSLogV(fmt, __VA_ARGS__);}
#endif

#endif // __DasLogMacros_H__
