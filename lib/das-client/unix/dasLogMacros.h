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

#include <stdio.h>

#define LOGD(...) {fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}

#ifdef NDEBUG
#define LOGV(...)
#else
#define LOGV(...) {fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}
#endif

#endif // __DasLogMacros_H__
