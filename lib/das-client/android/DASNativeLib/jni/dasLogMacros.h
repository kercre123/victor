/**
 * File: dasLogMacros
 *
 * Author: seichert
 * Created: 07/17/14
 *
 * Description: Macros for DAS implementation to log its own state
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef __DasLogMacros_H__
#define __DasLogMacros_H__

#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "daslib", __VA_ARGS__)

#ifdef NDEBUG
#define LOGV(...)
#else
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, "daslib", __VA_ARGS__)
#endif

#endif // __DasLogMacros_H__
